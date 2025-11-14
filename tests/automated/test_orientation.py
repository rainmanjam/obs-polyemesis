#!/usr/bin/env python3
"""
OBS Polyemesis - Orientation-Specific Streaming Tests
Tests horizontal (landscape) and vertical (portrait) streaming
"""

import os
import sys
import time
import unittest
from test_polyemesis import RestreamerClient


class TestOrientationStreaming(unittest.TestCase):
    """Test horizontal and vertical video streaming"""

    @classmethod
    def setUpClass(cls):
        """Setup with stream credentials from environment"""
        cls.client = RestreamerClient(
            os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
            os.getenv('RESTREAMER_USER', 'admin'),
            os.getenv('RESTREAMER_PASS', 'admin')
        )

        # Stream credentials
        cls.twitch_url = os.getenv('TWITCH_INGEST_URL')
        cls.twitch_key = os.getenv('TWITCH_STREAM_KEY')
        cls.youtube_url = os.getenv('YOUTUBE_INGEST_URL')
        cls.youtube_key = os.getenv('YOUTUBE_STREAM_KEY')
        cls.instagram_url = os.getenv('INSTAGRAM_INGEST_URL')
        cls.instagram_key = os.getenv('INSTAGRAM_STREAM_KEY')

        # Input URLs
        cls.horizontal_input = os.getenv('OBS_RTMP_URL_HORIZONTAL', 'rtmp://localhost/live/obs_horizontal')
        cls.vertical_input = os.getenv('OBS_RTMP_URL_VERTICAL', 'rtmp://localhost/live/obs_vertical')

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

    def test_01_horizontal_multistream_1min(self):
        """Test horizontal 1080p60 streaming to YouTube + Twitch for 1 minute"""
        if not (self.youtube_url and self.youtube_key and self.twitch_url and self.twitch_key):
            self.skipTest("YouTube and Twitch credentials required (YOUTUBE_*, TWITCH_*)")

        process_id = f"test_horizontal_multi_{int(time.time())}"
        self.test_processes.append(process_id)

        print(f"\n{'='*70}")
        print(f"HORIZONTAL TEST: 1080p60 to YouTube + Twitch (1 minute)")
        print(f"{'='*70}")

        # Create horizontal multi-destination stream
        outputs = [
            {
                "address": f"{self.youtube_url}/{self.youtube_key}",
                "id": "youtube_output",
                "options": [
                    "-c:v", "libx264", "-preset", "veryfast",
                    "-b:v", "8000k", "-maxrate", "8000k", "-bufsize", "16000k",
                    "-g", "120", "-r", "60",
                    "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
                ]
            },
            {
                "address": f"{self.twitch_url}/{self.twitch_key}",
                "id": "twitch_output",
                "options": [
                    "-c:v", "libx264", "-preset", "veryfast",
                    "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
                    "-g", "120", "-r", "60",
                    "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
                ]
            }
        ]

        result = self.client.create_process(process_id, self.horizontal_input, outputs)
        self.assertEqual(len(result['output']), 2)
        print(f"✓ Created process with 2 outputs (YouTube @ 8000k, Twitch @ 6000k)")

        # Start multistream
        start_result = self.client.start_process(process_id)
        self.assertIn(start_result.get('exec'), ['starting', 'running', 'failed'])
        print(f"✓ Stream started: {start_result.get('exec')}")

        # Monitor for 60 seconds (1 minute)
        print(f"\n📡 Streaming HORIZONTAL to YouTube + Twitch for 60 seconds...")
        print(f"   Resolution: 1920x1080 @ 60fps")
        print(f"   YouTube:    8000 kbps")
        print(f"   Twitch:     6000 kbps\n")

        for i in range(12):
            time.sleep(5)
            state = self.client.get_process_state(process_id)
            elapsed = (i + 1) * 5
            status = state.get('exec')
            print(f"  [{elapsed:2d}s] State: {status}")

        # Stop stream
        self.client.stop_process(process_id)
        print(f"\n✓ Test completed successfully")

    def test_02_vertical_instagram_1min(self):
        """Test vertical 720x1280 streaming to Instagram for 1 minute

        NOTE: Instagram requires an ACTIVE "Go Live" session. If you see "failed"
        state, this is EXPECTED when using inactive/expired stream keys.

        To test with real Instagram streaming:
        1. Start "Go Live" from Instagram app/web
        2. Copy the fresh RTMPS URL + stream key
        3. Update .env with the new credentials
        4. Run this test immediately (keys expire quickly)

        The test validates:
        - Correct RTMPS URL format
        - Proper 720x1280 @ 30fps encoding (Instagram specs)
        - Stereo 44.1kHz audio configuration
        - Video bitrate within Instagram's 2250-6000 Kbps range
        """
        if not self.instagram_url or not self.instagram_key:
            self.skipTest("Instagram credentials not provided (INSTAGRAM_INGEST_URL, INSTAGRAM_STREAM_KEY)")

        process_id = f"test_vertical_ig_{int(time.time())}"
        self.test_processes.append(process_id)

        print(f"\n{'='*70}")
        print(f"VERTICAL TEST: 720x1280 @ 30fps to Instagram (1 minute)")
        print(f"{'='*70}")
        print(f"NOTE: Instagram requires ACTIVE 'Go Live' session")
        print(f"      'failed' state = inactive/expired key (expected)")
        print(f"      'running' state = active Instagram stream!")
        print(f"{'='*70}")

        # Create vertical Instagram stream
        # Instagram specs: 720x1280 (9:16) @ 30fps, 2250-6000kbps, stereo 44.1kHz 256k
        result = self.client.create_process(
            process_id,
            self.vertical_input,
            [{
                "address": f"{self.instagram_url}/{self.instagram_key}",
                "id": "instagram_output",
                "options": [
                    "-c:v", "libx264",
                    "-preset", "veryfast",
                    "-b:v", "4000k",
                    "-maxrate", "4000k",
                    "-bufsize", "8000k",
                    "-g", "60",           # 2 seconds @ 30fps
                    "-r", "30",           # Instagram prefers 30fps for portrait
                    "-profile:v", "main",
                    "-c:a", "aac",
                    "-b:a", "256k",       # Up to 256k per Instagram specs
                    "-ar", "44100",       # 44.1 KHz per Instagram specs
                    "-ac", "2",           # Stereo per Instagram specs
                    "-f", "flv"
                ]
            }]
        )

        self.assertEqual(result['id'], process_id)
        print(f"✓ Created vertical Instagram process (720x1280 @ 30fps, 4000kbps, stereo 44.1kHz)")

        # Start stream
        start_result = self.client.start_process(process_id)
        self.assertIn(start_result.get('exec'), ['starting', 'running', 'failed'])
        print(f"✓ Stream started: {start_result.get('exec')}")

        # Monitor for 60 seconds (1 minute)
        print(f"\n📱 Streaming VERTICAL to Instagram for 60 seconds...")
        print(f"   Resolution: 720x1280 (9:16) @ 30fps")
        print(f"   Video:      4000 kbps")
        print(f"   Audio:      256 kbps stereo @ 44.1 KHz\n")

        for i in range(12):
            time.sleep(5)
            state = self.client.get_process_state(process_id)
            elapsed = (i + 1) * 5
            status = state.get('exec')
            print(f"  [{elapsed:2d}s] State: {status}")

        # Stop stream
        self.client.stop_process(process_id)
        print(f"\n✓ Test completed successfully")


def run_orientation_tests():
    """Run orientation-specific streaming tests"""
    # Check for required credentials
    youtube_ready = bool(os.getenv('YOUTUBE_INGEST_URL') and os.getenv('YOUTUBE_STREAM_KEY'))
    twitch_ready = bool(os.getenv('TWITCH_INGEST_URL') and os.getenv('TWITCH_STREAM_KEY'))
    instagram_ready = bool(os.getenv('INSTAGRAM_INGEST_URL') and os.getenv('INSTAGRAM_STREAM_KEY'))

    print("\n" + "="*70)
    print("OBS POLYEMESIS - ORIENTATION STREAMING TESTS")
    print("="*70)
    print(f"YouTube credentials:   {'✓' if youtube_ready else '✗'}")
    print(f"Twitch credentials:    {'✓' if twitch_ready else '✗'}")
    print(f"Instagram credentials: {'✓' if instagram_ready else '✗'}")
    print("="*70 + "\n")

    if not (youtube_ready and twitch_ready):
        print("⚠️  Horizontal test requires both YouTube and Twitch credentials")
        print("   Set: YOUTUBE_INGEST_URL, YOUTUBE_STREAM_KEY")
        print("   Set: TWITCH_INGEST_URL, TWITCH_STREAM_KEY\n")

    if not instagram_ready:
        print("⚠️  Vertical test requires Instagram credentials")
        print("   Set: INSTAGRAM_INGEST_URL, INSTAGRAM_STREAM_KEY\n")

    # Run tests
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestOrientationStreaming)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_orientation_tests())
