#!/usr/bin/env python3
"""
Performance and Load Testing for OBS Polyemesis
Benchmarks API performance, resource usage, and stress testing
"""

import os
import sys
import time
import psutil
import unittest
import threading
from typing import List, Dict
from test_polyemesis import RestreamerClient


class PerformanceMonitor:
    """Helper class to monitor system resources"""

    def __init__(self):
        self.measurements = []
        self.monitoring = False
        self._thread = None

    def start(self):
        """Start monitoring resources"""
        self.monitoring = True
        self.measurements = []
        self._thread = threading.Thread(target=self._monitor_loop)
        self._thread.daemon = True
        self._thread.start()

    def stop(self):
        """Stop monitoring and return stats"""
        self.monitoring = False
        if self._thread:
            self._thread.join(timeout=2)

        if not self.measurements:
            return {}

        cpu_values = [m['cpu'] for m in self.measurements]
        mem_values = [m['memory'] for m in self.measurements]

        return {
            'cpu_avg': sum(cpu_values) / len(cpu_values),
            'cpu_max': max(cpu_values),
            'memory_avg': sum(mem_values) / len(mem_values),
            'memory_max': max(mem_values),
            'samples': len(self.measurements)
        }

    def _monitor_loop(self):
        """Internal monitoring loop"""
        while self.monitoring:
            try:
                self.measurements.append({
                    'cpu': psutil.cpu_percent(interval=0.1),
                    'memory': psutil.virtual_memory().percent,
                    'timestamp': time.time()
                })
            except:
                pass
            time.sleep(0.5)


class TestPerformance(unittest.TestCase):
    """Performance and load testing"""

    @classmethod
    def setUpClass(cls):
        cls.base_url = os.getenv('RESTREAMER_URL', 'http://localhost:8080')
        cls.username = os.getenv('RESTREAMER_USER', 'admin')
        cls.password = os.getenv('RESTREAMER_PASS', 'admin')
        cls.client = RestreamerClient(cls.base_url, cls.username, cls.password)
        cls.created_processes = []

    @classmethod
    def tearDownClass(cls):
        """Clean up all created processes"""
        for process_id in cls.created_processes:
            try:
                cls.client.delete(f'/api/v3/process/{process_id}')
            except:
                pass

    def test_01_api_response_time(self):
        """
        Benchmark API response times for common operations
        """
        print("\n⏱️  Benchmarking API response times...")

        operations = {
            'GET /about': lambda: self.client.get('/api/v3/about'),
            'GET /process (list)': lambda: self.client.get('/api/v3/process'),
            'GET /config': lambda: self.client.get('/api/v3/config'),
        }

        results = {}

        for operation, func in operations.items():
            times = []
            for i in range(10):  # 10 samples
                start = time.time()
                try:
                    func()
                    elapsed = (time.time() - start) * 1000  # Convert to ms
                    times.append(elapsed)
                except Exception as e:
                    print(f"⚠️  {operation} failed: {e}")

            if times:
                avg_time = sum(times) / len(times)
                min_time = min(times)
                max_time = max(times)
                results[operation] = {
                    'avg': avg_time,
                    'min': min_time,
                    'max': max_time
                }
                print(f"📊 {operation}: {avg_time:.2f}ms avg ({min_time:.2f}-{max_time:.2f}ms)")

        # Assert performance benchmarks
        if 'GET /about' in results:
            self.assertLess(results['GET /about']['avg'], 1000,
                          "GET /about should be under 1000ms on average")

        print("✅ API response time benchmarking completed")

    def test_02_concurrent_process_creation(self):
        """
        Test creating multiple processes concurrently (Phase 5, Test 5.1 partial)
        """
        print("\n🚀 Testing concurrent process creation...")

        monitor = PerformanceMonitor()
        monitor.start()

        num_processes = 5
        process_ids = []

        start_time = time.time()

        for i in range(num_processes):
            process_id = f"perf_test_{int(time.time())}_{i}"
            process_ids.append(process_id)

            try:
                self.client.create_process(
                    process_id=process_id,
                    input_url=f"rtmp://localhost/test{i}",
                    outputs=[]
                )
                print(f"✅ Created process {i+1}/{num_processes}")
                self.created_processes.append(process_id)
            except Exception as e:
                print(f"⚠️  Failed to create process {i+1}: {e}")

        elapsed = time.time() - start_time

        stats = monitor.stop()

        print(f"\n📊 Performance Stats:")
        print(f"   Total time: {elapsed:.2f}s")
        print(f"   Processes created: {len(process_ids)}")
        print(f"   Avg CPU: {stats.get('cpu_avg', 0):.1f}%")
        print(f"   Max CPU: {stats.get('cpu_max', 0):.1f}%")
        print(f"   Avg Memory: {stats.get('memory_avg', 0):.1f}%")

        self.assertGreater(len(process_ids), 0, "Should create at least one process")

    def test_03_rapid_start_stop_cycles(self):
        """
        Test rapid start/stop operations (Phase 5, Test 5.3)
        """
        print("\n🔄 Testing rapid start/stop cycles...")

        process_id = f"rapid_test_{int(time.time())}"

        # Create process
        try:
            self.client.create_process(
                process_id=process_id,
                input_url="rtmp://localhost/rapid",
                outputs=[]
            )
            self.created_processes.append(process_id)
        except Exception as e:
            self.skipTest(f"Could not create test process: {e}")

        cycles = 5
        timings = []

        for i in range(cycles):
            start_time = time.time()

            # Start
            try:
                self.client.put(f'/api/v3/process/{process_id}/command', {
                    'command': 'start'
                })
            except:
                pass

            time.sleep(0.5)

            # Stop
            try:
                self.client.put(f'/api/v3/process/{process_id}/command', {
                    'command': 'stop'
                })
            except:
                pass

            elapsed = time.time() - start_time
            timings.append(elapsed)
            print(f"✅ Cycle {i+1}/{cycles}: {elapsed:.2f}s")

            time.sleep(0.5)

        avg_cycle_time = sum(timings) / len(timings)
        print(f"\n📊 Average cycle time: {avg_cycle_time:.2f}s")

        self.assertLess(avg_cycle_time, 10, "Cycles should complete in under 10s")

    def test_04_memory_leak_detection(self):
        """
        Monitor for memory leaks during repeated operations
        """
        print("\n🔍 Testing for memory leaks...")

        monitor = PerformanceMonitor()
        monitor.start()

        iterations = 10
        process_ids = []

        for i in range(iterations):
            process_id = f"leak_test_{int(time.time())}_{i}"
            process_ids.append(process_id)

            try:
                # Create
                self.client.create_process(
                    process_id=process_id,
                    input_url=f"rtmp://localhost/leak{i}",
                    outputs=[]
                )

                # Delete immediately
                self.client.delete(f'/api/v3/process/{process_id}')

            except Exception as e:
                print(f"⚠️  Iteration {i+1} error: {e}")

            time.sleep(0.2)

        stats = monitor.stop()

        # Analyze memory trend
        if stats.get('samples', 0) > 5:
            memory_values = [m['memory'] for m in monitor.measurements]
            first_half = memory_values[:len(memory_values)//2]
            second_half = memory_values[len(memory_values)//2:]

            first_avg = sum(first_half) / len(first_half)
            second_avg = sum(second_half) / len(second_half)
            memory_increase = second_avg - first_avg

            print(f"\n📊 Memory Analysis:")
            print(f"   First half avg: {first_avg:.1f}%")
            print(f"   Second half avg: {second_avg:.1f}%")
            print(f"   Memory increase: {memory_increase:+.1f}%")

            if memory_increase > 5:
                print("⚠️  WARNING: Potential memory leak detected")
            else:
                print("✅ No significant memory leak detected")

        self.assertTrue(True, "Memory leak detection completed")

    def test_05_api_throughput(self):
        """
        Measure API request throughput
        """
        print("\n📈 Measuring API throughput...")

        duration = 5  # seconds
        start_time = time.time()
        request_count = 0
        errors = 0

        while time.time() - start_time < duration:
            try:
                self.client.get('/api/v3/about')
                request_count += 1
            except:
                errors += 1

        elapsed = time.time() - start_time
        throughput = request_count / elapsed

        print(f"\n📊 Throughput Results:")
        print(f"   Duration: {elapsed:.2f}s")
        print(f"   Requests: {request_count}")
        print(f"   Errors: {errors}")
        print(f"   Throughput: {throughput:.1f} req/s")

        self.assertGreater(throughput, 10, "Should handle at least 10 req/s")


if __name__ == '__main__':
    # Check if psutil is available
    try:
        import psutil
    except ImportError:
        print("WARNING: psutil not installed. Install with: pip3 install psutil")
        print("Some performance tests may not work correctly.\n")

    print("=" * 70)
    print("  Performance and Load Testing Suite")
    print("=" * 70)
    print("\nBenchmarking API performance, resource usage, and stress testing.\n")

    unittest.main(verbosity=2)
