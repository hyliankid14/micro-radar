#pragma once

#include <Arduino.h>
#include <deque>
#include <functional>
#include <set>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Single background worker that executes every blocking network job (API calls,
// image downloads) off the UI thread. Jobs run strictly one at a time, so the
// shared HTTP clients never need their own locking. The UI thread only submits
// jobs and later reads their results from mutex-guarded caches.
class NetWorker
{
public:
    // Lower value = runs sooner. Equal priorities run FIFO.
    enum Priority : int { PRIO_URGENT = 0, PRIO_VISIBLE = 1, PRIO_PREFETCH = 2, PRIO_BACKGROUND = 3 };

private:
    struct Job {
        String key;
        int prio;
        std::function<void()> fn;
    };

    std::deque<Job> queue;
    std::set<String> inflight; // keys that are queued or currently running
    SemaphoreHandle_t mutex = nullptr;
    TaskHandle_t task = nullptr;
    volatile uint32_t jobsRun = 0;
    volatile uint32_t lastJobMs = 0;
    volatile uint32_t maxJobMs = 0;

    static void TaskEntry(void* arg) { static_cast<NetWorker*>(arg)->Run(); }

    void Run()
    {
        for (;;) {
            Job job;
            bool have = false;
            xSemaphoreTake(mutex, portMAX_DELAY);
            if (!queue.empty()) {
                job = std::move(queue.front());
                queue.pop_front();
                have = true;
            }
            xSemaphoreGive(mutex);

            if (!have) {
                vTaskDelay(pdMS_TO_TICKS(25));
                continue;
            }

            const uint32_t t0 = millis();
            job.fn();
            const uint32_t dt = millis() - t0;

            xSemaphoreTake(mutex, portMAX_DELAY);
            inflight.erase(job.key);
            xSemaphoreGive(mutex);

            jobsRun = jobsRun + 1;
            lastJobMs = dt;
            if (dt > maxJobMs) maxJobMs = dt;
            if (dt > 1500) Serial.printf("[PERF] slow net job '%s' took %lu ms\n", job.key.c_str(), static_cast<unsigned long>(dt));

            // Yield so the UI/WiFi stack stay responsive between jobs
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }

public:
    void Begin()
    {
        if (task) return;
        mutex = xSemaphoreCreateMutex();
        // Core 0 hosts WiFi/LwIP; the Arduino loop (UI) runs on core 1.
        xTaskCreatePinnedToCore(TaskEntry, "net-worker", 24576, this, 1, &task, 0);
    }

    // Returns false if a job with the same key is already queued/running.
    bool Submit(const String& key, std::function<void()> fn, int prio = PRIO_VISIBLE)
    {
        if (!mutex) return false;
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (inflight.count(key)) {
            // Promote an already-queued job if the new request is more urgent
            for (auto& j : queue) {
                if (j.key == key && prio < j.prio) j.prio = prio;
            }
            xSemaphoreGive(mutex);
            return false;
        }
        inflight.insert(key);
        auto pos = queue.end();
        while (pos != queue.begin() && (pos - 1)->prio > prio) --pos;
        queue.insert(pos, Job{key, prio, std::move(fn)});
        xSemaphoreGive(mutex);
        return true;
    }

    bool IsInflight(const String& key)
    {
        if (!mutex) return false;
        xSemaphoreTake(mutex, portMAX_DELAY);
        bool r = inflight.count(key) > 0;
        xSemaphoreGive(mutex);
        return r;
    }

    size_t Pending()
    {
        if (!mutex) return 0;
        xSemaphoreTake(mutex, portMAX_DELAY);
        size_t n = inflight.size();
        xSemaphoreGive(mutex);
        return n;
    }

    uint32_t JobsRun() const { return jobsRun; }
    uint32_t LastJobMs() const { return lastJobMs; }
    uint32_t MaxJobMs() const { return maxJobMs; }
};
