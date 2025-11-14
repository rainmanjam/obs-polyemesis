#!/usr/bin/env python3
"""
OBS Polyemesis - Live Streaming Tests
Tests actual streaming to platforms (requires valid stream keys)
"""

import os
import sys
import time
import unittest
from test_polyemesis import RestreamerClient


class TestLiveStreaming(unittest.TestCase):
    """Test live streaming to actual platforms"""

    @classmethod
    def setUpClass(cls):
        """Setup with stream credentials from environment"""
        cls.client = RestreamerClient(
            os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
            os.getenv('RESTREAMER_USER', 'admin'),
            os.getenv('RESTREAMER_PASS', 'admin')
        )

        # Stream credentials (must be provided via environment)
        cls.twitch_url = os.getenv('TWITCH_INGEST_URL')
        cls.twitch_key = os.getenv('TWITCH_STREAM_KEY')
        cls.youtube_url = os.getenv('YOUTUBE_INGEST_URL')
        cls.youtube_key = os.getenv('YOUTUBE_STREAM_KEY')

        # OBS input URL
        cls.obs_input = os.getenv('OBS_RTMP_URL', 'rtmp://localhost/live/obs_output')

        cls.test_processes = []

    @classmethod
    def tearDownClass(cls):
        """Cleanup all test streams"""
        for process_id in cls.test_processes:
            try:
                cls.client.stop_process(process_id)
                time.sleep(2)
                cls.client.delete_process(process_id)
            except Exception:
                # Ignore cleanup errors - process may already be stopped/deleted
                pass

    def test_01_twitch_stream(self):
        """Test streaming to Twitch"""
        if not self.twitch_url or not self.twitch_key:
            self.skipTest("Twitch credentials not provided (TWITCH_INGEST_URL, TWITCH_STREAM_KEY)")

        process_id = f"test_twitch_{int(time.time())}"
        self.test_processes.append(process_id)

        # Create Twitch stream - 1080p60 6000kbps
        result = self.client.create_process(
            process_id,
            self.obs_input,
            [{
                "address": f"{self.twitch_url}/{self.twitch_key}",
                "id": "twitch_output",
                "options": [
                    "-c:v", "libx264",
                    "-preset", "veryfast",
                    "-b:v", "6000k",
                    "-maxrate", "6000k",
                    "-bufsize", "12000k",
                    "-g", "120",
                    "-r", "60",
                    "-c:a", "aac",
                    "-b:a", "160k",
                    "-ar", "48000",
                    "-f", "flv"
                ]
            }]
        )

        self.assertEqual(result['id'], process_id)

        # Start stream
        start_result = self.client.start_process(process_id)
        # datarhei-core v16.16.0: state response has 'exec' field
        self.assertIn(start_result.get('exec'), ['starting', 'running', 'failed'])

        # Monitor for 30 seconds
        print(f"\nStreaming to Twitch for 30 seconds...")
        for i in range(6):
            time.sleep(5)
            state = self.client.get_process_state(process_id)
            # datarhei-core v16.16.0: state response has 'exec' field
            print(f"  State: {state.get('exec')}")

        # Stop stream
        self.client.stop_process(process_id)

    def test_02_youtube_stream(self):
        """Test streaming to YouTube"""
        if not self.youtube_url or not self.youtube_key:
            self.skipTest("YouTube credentials not provided (YOUTUBE_INGEST_URL, YOUTUBE_STREAM_KEY)")

        process_id = f"test_youtube_{int(time.time())}"
        self.test_processes.append(process_id)

        # Create YouTube stream - 1080p60 8000kbps
        result = self.client.create_process(
            process_id,
            self.obs_input,
            [{
                "address": f"{self.youtube_url}/{self.youtube_key}",
                "id": "youtube_output",
                "options": [
                    "-c:v", "libx264",
                    "-preset", "veryfast",
                    "-b:v", "8000k",
                    "-maxrate", "8000k",
                    "-bufsize", "16000k",
                    "-g", "120",
                    "-r", "60",
                    "-c:a", "aac",
                    "-b:a", "160k",
                    "-ar", "48000",
                    "-f", "flv"
                ]
            }]
        )

        self.assertEqual(result['id'], process_id)

        # Start stream
        start_result = self.client.start_process(process_id)
        # datarhei-core v16.16.0: state response has 'exec' field
        self.assertIn(start_result.get('exec'), ['starting', 'running', 'failed'])

        # Monitor for 30 seconds
        print(f"\nStreaming to YouTube for 30 seconds...")
        for i in range(6):
            time.sleep(5)
            state = self.client.get_process_state(process_id)
            # datarhei-core v16.16.0: state response has 'exec' field
            print(f"  State: {state.get('exec')}")

        # Stop stream
        self.client.stop_process(process_id)

    def test_03_multistream(self):
        """Test 3.7: Simultaneous streaming to multiple platforms"""
        if not (self.twitch_url and self.twitch_key and self.youtube_url and self.youtube_key):
            self.skipTest("Both Twitch and YouTube credentials required")

        process_id = f"test_multi_{int(time.time())}"
        self.test_processes.append(process_id)

        # Create multi-destination stream - 1080p60
        outputs = [
            {
                "address": f"{self.twitch_url}/{self.twitch_key}",
                "id": "twitch_output",
                "options": [
                    "-c:v", "libx264", "-preset", "veryfast",
                    "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
                    "-g", "120", "-r", "60",
                    "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
                ]
            },
            {
                "address": f"{self.youtube_url}/{self.youtube_key}",
                "id": "youtube_output",
                "options": [
                    "-c:v", "libx264", "-preset", "veryfast",
                    "-b:v", "8000k", "-maxrate", "8000k", "-bufsize", "16000k",
                    "-g", "120", "-r", "60",
                    "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
                ]
            }
        ]

        result = self.client.create_process(process_id, self.obs_input, outputs)
        self.assertEqual(len(result['output']), 2)

        # Start multistream
        start_result = self.client.start_process(process_id)
        # datarhei-core v16.16.0: state response has 'exec' field
        self.assertIn(start_result.get('exec'), ['starting', 'running', 'failed'])

        # Monitor for 60 seconds
        print(f"\nMultistreaming to Twitch + YouTube for 60 seconds...")
        for i in range(12):
            time.sleep(5)
            state = self.client.get_process_state(process_id)
            # datarhei-core v16.16.0: state response has 'exec' field
            print(f"  State: {state.get('exec')}")

        # Stop stream
        self.client.stop_process(process_id)


def run_live_tests():
    """Run live streaming tests"""
    # Check for required credentials
    required_env = ['TWITCH_INGEST_URL', 'TWITCH_STREAM_KEY', 'YOUTUBE_INGEST_URL', 'YOUTUBE_STREAM_KEY']
    missing = [env for env in required_env if not os.getenv(env)]

    if missing:
        print("WARNING: Missing environment variables:")
        for var in missing:
            print(f"  - {var}")
        print("\nSome tests will be skipped. To run all tests, set:")
        print("  export TWITCH_INGEST_URL='rtmp://live.twitch.tv/app'")
        print("  export TWITCH_STREAM_KEY='your_stream_key'")
        print("  export YOUTUBE_INGEST_URL='rtmp://a.rtmp.youtube.com/live2'")
        print("  export YOUTUBE_STREAM_KEY='your_stream_key'")
        print()

    # Run tests
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestLiveStreaming)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_live_tests())
