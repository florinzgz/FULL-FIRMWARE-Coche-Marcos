#!/usr/bin/env python3
"""
PHASE 11 — Telemetry Data Collector & Analyzer

This script helps automate the collection and analysis of graphics telemetry
data from the ESP32-S3 serial output during Phase 11 validation testing.

**IMPORTANT NOTE:**
The current firmware implementation displays telemetry data on-screen only
(via the graphics telemetry overlay). It does NOT output telemetry to serial.

To use this script, you have two options:

1. **Manual CSV Creation:**
   - Manually record telemetry values from the on-screen display
   - Create a CSV file with the format shown in TelemetryFrame structure
   - Use the --analyze option to process your CSV file

2. **Add Serial Logging (requires code modification):**
   - Modify src/hud/hud_graphics_telemetry.cpp to add Serial.printf()
   - Output format: [TELEMETRY] FPS:X Frame:Yms DirtyRects:Z Bytes:WKB Shadow:ON/OFF Errors:E
   - Rebuild and flash firmware
   - Then use this script for real-time monitoring

For most users, Option 1 (manual CSV) is recommended.

Features:
- Real-time serial monitoring with telemetry extraction (if serial logging added)
- Statistical analysis (min/max/avg)
- CSV export for further analysis
- Pass/fail criteria validation
- Summary report generation

Usage:
    # Analyze manually created CSV file (RECOMMENDED)
    python3 phase11_telemetry_collector.py --analyze telemetry_data.csv
    
    # Monitor serial port (requires serial logging modification)
    python3 phase11_telemetry_collector.py --port /dev/ttyUSB0 --duration 60
    python3 phase11_telemetry_collector.py --port COM4 --test idle
"""

import argparse
import csv
import re
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from statistics import mean, median
from typing import List, Optional

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False
    print("Warning: pyserial not installed. Live monitoring not available.")
    print("Install with: pip install pyserial")
    print("Note: Install 'pyserial' NOT 'serial' - they are different packages!")


@dataclass
class TelemetryFrame:
    """Represents a single frame of telemetry data."""
    timestamp: float
    fps: int
    frame_time_ms: int
    dirty_rects: int
    bytes_kb: float
    shadow_enabled: bool
    shadow_errors: int
    
    def to_dict(self):
        return {
            'timestamp': self.timestamp,
            'fps': self.fps,
            'frame_time_ms': self.frame_time_ms,
            'dirty_rects': self.dirty_rects,
            'bytes_kb': self.bytes_kb,
            'shadow_enabled': self.shadow_enabled,
            'shadow_errors': self.shadow_errors
        }


class TelemetryCollector:
    """Collects and analyzes graphics telemetry data."""
    
    def __init__(self):
        self.frames: List[TelemetryFrame] = []
        self.start_time: Optional[float] = None
        
    def parse_telemetry_line(self, line: str) -> Optional[TelemetryFrame]:
        """
        Parse a telemetry data line from serial output.
        
        Note: This script expects a specific telemetry output format that is NOT
        currently implemented in the firmware. The existing graphics telemetry
        displays data on-screen only.
        
        To use this script, you would need to add serial logging to
        src/hud/hud_graphics_telemetry.cpp or manually create CSV files.
        
        Expected format (example):
        [TELEMETRY] FPS:28 Frame:35ms DirtyRects:3 Bytes:18KB Shadow:ON Errors:0
        """
        # Try to match telemetry pattern
        pattern = r'\[TELEMETRY\]\s*FPS:(\d+)\s+Frame:(\d+)ms\s+DirtyRects:(\d+)\s+Bytes:([\d.]+)KB\s+Shadow:(ON|OFF)\s+Errors:(\d+)'
        match = re.search(pattern, line)
        
        if match:
            timestamp = time.time() - (self.start_time or time.time())
            return TelemetryFrame(
                timestamp=timestamp,
                fps=int(match.group(1)),
                frame_time_ms=int(match.group(2)),
                dirty_rects=int(match.group(3)),
                bytes_kb=float(match.group(4)),
                shadow_enabled=(match.group(5) == 'ON'),
                shadow_errors=int(match.group(6))
            )
        return None
    
    def add_frame(self, frame: TelemetryFrame):
        """Add a telemetry frame to the collection."""
        if self.start_time is None:
            self.start_time = time.time()
        self.frames.append(frame)
    
    def get_statistics(self) -> dict:
        """Calculate statistics from collected frames."""
        if not self.frames:
            return {}
        
        fps_values = [f.fps for f in self.frames]
        frame_time_values = [f.frame_time_ms for f in self.frames]
        dirty_rect_values = [f.dirty_rects for f in self.frames]
        bytes_values = [f.bytes_kb for f in self.frames]
        total_shadow_errors = sum(f.shadow_errors for f in self.frames)
        
        return {
            'total_frames': len(self.frames),
            'duration_sec': self.frames[-1].timestamp if self.frames else 0,
            'fps': {
                'min': min(fps_values),
                'max': max(fps_values),
                'avg': mean(fps_values),
                'median': median(fps_values)
            },
            'frame_time_ms': {
                'min': min(frame_time_values),
                'max': max(frame_time_values),
                'avg': mean(frame_time_values),
                'median': median(frame_time_values)
            },
            'dirty_rects': {
                'min': min(dirty_rect_values),
                'max': max(dirty_rect_values),
                'avg': mean(dirty_rect_values),
                'median': median(dirty_rect_values)
            },
            'bytes_kb': {
                'min': min(bytes_values),
                'max': max(bytes_values),
                'avg': mean(bytes_values),
                'median': median(bytes_values)
            },
            'shadow_errors': total_shadow_errors
        }
    
    def export_csv(self, filename: str):
        """Export collected frames to CSV file."""
        if not self.frames:
            print("No frames to export")
            return
        
        with open(filename, 'w', newline='') as csvfile:
            fieldnames = ['timestamp', 'fps', 'frame_time_ms', 'dirty_rects', 
                         'bytes_kb', 'shadow_enabled', 'shadow_errors']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for frame in self.frames:
                writer.writerow(frame.to_dict())
        
        print(f"Exported {len(self.frames)} frames to {filename}")
    
    def import_csv(self, filename: str):
        """Import frames from CSV file."""
        self.frames = []
        with open(filename, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                frame = TelemetryFrame(
                    timestamp=float(row['timestamp']),
                    fps=int(row['fps']),
                    frame_time_ms=int(row['frame_time_ms']),
                    dirty_rects=int(row['dirty_rects']),
                    bytes_kb=float(row['bytes_kb']),
                    shadow_enabled=(row['shadow_enabled'].lower() == 'true'),
                    shadow_errors=int(row['shadow_errors'])
                )
                self.frames.append(frame)
        print(f"Imported {len(self.frames)} frames from {filename}")
    
    def print_statistics(self):
        """Print formatted statistics report."""
        stats = self.get_statistics()
        
        if not stats:
            print("No telemetry data collected")
            return
        
        print("\n" + "="*60)
        print("PHASE 11 TELEMETRY STATISTICS REPORT")
        print("="*60)
        print(f"Total Frames: {stats['total_frames']}")
        print(f"Duration: {stats['duration_sec']:.1f} seconds")
        print()
        
        print("FPS (Frames Per Second):")
        print(f"  Minimum:  {stats['fps']['min']} fps")
        print(f"  Maximum:  {stats['fps']['max']} fps")
        print(f"  Average:  {stats['fps']['avg']:.1f} fps")
        print(f"  Median:   {stats['fps']['median']:.1f} fps")
        print()
        
        print("Frame Time:")
        print(f"  Minimum:  {stats['frame_time_ms']['min']} ms")
        print(f"  Maximum:  {stats['frame_time_ms']['max']} ms")
        print(f"  Average:  {stats['frame_time_ms']['avg']:.1f} ms")
        print(f"  Median:   {stats['frame_time_ms']['median']:.1f} ms")
        print()
        
        print("Dirty Rectangles:")
        print(f"  Minimum:  {stats['dirty_rects']['min']}")
        print(f"  Maximum:  {stats['dirty_rects']['max']}")
        print(f"  Average:  {stats['dirty_rects']['avg']:.1f}")
        print(f"  Median:   {stats['dirty_rects']['median']:.1f}")
        print()
        
        print("Bytes per Frame:")
        print(f"  Minimum:  {stats['bytes_kb']['min']:.1f} KB")
        print(f"  Maximum:  {stats['bytes_kb']['max']:.1f} KB")
        print(f"  Average:  {stats['bytes_kb']['avg']:.1f} KB")
        print(f"  Median:   {stats['bytes_kb']['median']:.1f} KB")
        print()
        
        print("Shadow Validation:")
        print(f"  Total Errors: {stats['shadow_errors']}")
        print()
        
        self.print_pass_fail(stats)
    
    def print_pass_fail(self, stats: dict):
        """Evaluate and print pass/fail criteria."""
        print("="*60)
        print("PASS/FAIL CRITERIA EVALUATION")
        print("="*60)
        
        # Criteria checks
        fps_pass = stats['fps']['avg'] >= 25
        bytes_pass = stats['bytes_kb']['avg'] < 60
        shadow_pass = stats['shadow_errors'] == 0
        
        print(f"{'✅' if fps_pass else '❌'} Average FPS ≥ 25: {stats['fps']['avg']:.1f} fps")
        print(f"{'✅' if bytes_pass else '❌'} Average Bytes < 60 KB: {stats['bytes_kb']['avg']:.1f} KB")
        print(f"{'✅' if shadow_pass else '❌'} Shadow Errors = 0: {stats['shadow_errors']}")
        
        overall_pass = fps_pass and bytes_pass and shadow_pass
        
        print()
        if overall_pass:
            print("🎉 OVERALL: PASS ✅")
        else:
            print("⚠️ OVERALL: FAIL ❌")
        print("="*60)
        print()


def monitor_serial(port: str, baudrate: int, duration: Optional[int], output_csv: Optional[str]):
    """Monitor serial port and collect telemetry data."""
    if not SERIAL_AVAILABLE:
        print("ERROR: pyserial not installed")
        return
    
    collector = TelemetryCollector()
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud")
        print("Collecting telemetry data...")
        if duration:
            print(f"Collection will run for {duration} seconds")
        print("Press Ctrl+C to stop\n")
        
        start_time = time.time()
        
        while True:
            if duration and (time.time() - start_time) > duration:
                print("\nDuration reached, stopping collection")
                break
            
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    # Print raw line for debugging
                    if '[TELEMETRY]' in line:
                        print(line)
                    
                    # Try to parse telemetry
                    frame = collector.parse_telemetry_line(line)
                    if frame:
                        collector.add_frame(frame)
            except KeyboardInterrupt:
                print("\nStopping collection...")
                break
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return
    
    # Print statistics
    if collector.frames:
        collector.print_statistics()
        
        # Export to CSV if requested
        if output_csv:
            collector.export_csv(output_csv)
    else:
        print("No telemetry data collected")
        print("\nTroubleshooting:")
        print("1. Verify Hidden Menu is open on device")
        print("2. Check that graphics telemetry is enabled")
        print("3. Confirm serial output format matches expected pattern")


def analyze_csv(filename: str):
    """Analyze telemetry data from CSV file."""
    collector = TelemetryCollector()
    
    try:
        collector.import_csv(filename)
        collector.print_statistics()
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
    except Exception as e:
        print(f"Error analyzing CSV: {e}")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description='Phase 11 Telemetry Data Collector & Analyzer',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Monitor serial port for 60 seconds
  python3 phase11_telemetry_collector.py --port /dev/ttyUSB0 --duration 60
  
  # Monitor and export to CSV
  python3 phase11_telemetry_collector.py --port COM4 --output telemetry.csv
  
  # Analyze existing CSV file
  python3 phase11_telemetry_collector.py --analyze telemetry.csv
        '''
    )
    
    parser.add_argument('--port', help='Serial port (e.g., /dev/ttyUSB0 or COM4)')
    parser.add_argument('--baudrate', type=int, default=115200, 
                       help='Serial baudrate (default: 115200)')
    parser.add_argument('--duration', type=int, 
                       help='Collection duration in seconds (optional)')
    parser.add_argument('--output', help='Output CSV filename')
    parser.add_argument('--analyze', help='Analyze CSV file instead of live monitoring')
    
    args = parser.parse_args()
    
    if args.analyze:
        analyze_csv(args.analyze)
    elif args.port:
        monitor_serial(args.port, args.baudrate, args.duration, args.output)
    else:
        parser.print_help()
        print("\nERROR: Either --port or --analyze must be specified")
        sys.exit(1)


if __name__ == '__main__':
    main()
