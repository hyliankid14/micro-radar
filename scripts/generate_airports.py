#!/usr/bin/env python3
"""Generate AirportData.h from OpenFlights airport database."""

import csv

input_file = '/tmp/airports.csv'
output_file = '/Users/shaivure/Development/ESP32/Aircraft Tracker/src/AirportData.h'

airports = {}  # iata -> name

with open(input_file, 'r', encoding='utf-8') as f:
    reader = csv.reader(f)
    for row in reader:
        if len(row) < 5:
            continue
        # Format: ID,Name,City,Country,IATA,ICAO,Latitude,Longitude,Altitude,Timezone,DST,TZ,Type,Source
        name = row[1].strip()
        iata = row[4].strip()
        
        # Skip airports with no IATA code
        if not iata or iata == '' or iata == '\\N' or len(iata) != 3:
            continue
        
        # Skip if already have this IATA (keep first occurrence)
        if iata in airports:
            continue
        
        # Clean name: remove non-ASCII, truncate, escape quotes
        clean_name = ''.join(c for c in name if c.isascii() and c.isprintable())
        clean_name = clean_name.replace('"', '\\"').strip()
        if len(clean_name) > 70:
            clean_name = clean_name[:67] + '...'
        
        if not clean_name:
            continue
        
        airports[iata] = clean_name

# Sort by IATA code
sorted_codes = sorted(airports.keys())

with open(output_file, 'w') as f:
    f.write('// Static airport database for Micro Radar\n')
    f.write('// Sorted alphabetically by IATA code - generated from OpenFlights\n')
    f.write(f'// {len(sorted_codes)} airports worldwide\n\n')
    f.write('#pragma once\n\n')
    f.write('#include <Arduino.h>\n\n')
    f.write('struct AirportData {\n')
    f.write('    const char* iata;\n')
    f.write('    const char* name;\n')
    f.write('};\n\n')
    f.write('#define AP(CODE, NAME) { CODE, NAME }\n\n')
    f.write('static const AirportData AIRPORTS[] PROGMEM = {\n')
    
    for code in sorted_codes:
        name = airports[code]
        f.write(f'    AP("{code}", "{name}"),\n')
    
    f.write('};\n\n')
    f.write(f'constexpr int AIRPORTS_COUNT = sizeof(AIRPORTS) / sizeof(AIRPORTS[0]);\n')

print(f"Generated {len(sorted_codes)} airports")
