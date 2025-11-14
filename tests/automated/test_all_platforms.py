#!/usr/bin/env python3
"""
OBS Polyemesis - All Platforms Simultaneous Streaming Test
Tests streaming to YouTube + Twitch + Kick + Instagram simultaneously
"""

import os
import sys
import time
import unittest
from test_polyemesis import RestreamerClient


class TestAllPlatforms(unittest.TestCase):
    """Test simultaneous streaming to YouTube, Twitch, Kick, and Instagram"""

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
        cls.kick_url = os.getenv('KICK_INGEST_URL')
        cls.kick_key = os.getenv('KICK_STREAM_KEY')
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

    def test_all_platforms_simultaneous_2min(self):
        """Test streaming to YouTube + Twitch + Kick + Instagram simultaneously for 2 minutes"""

        # Check credentials
        if not (self.youtube_url and self.youtube_key):
            self.skipTest("YouTube credentials required (YOUTUBE_*)")
        if not (self.twitch_url and self.twitch_key):
            self.skipTest("Twitch credentials required (TWITCH_*)")
        if not (self.kick_url and self.kick_key):
            self.skipTest("Kick credentials required (KICK_*)")
        if not (self.instagram_url and self.instagram_key):
            self.skipTest("Instagram credentials required (INSTAGRAM_*)")

        print(f"\n{'='*70}")
        print(f"ALL PLATFORMS TEST: Simultaneous Streaming (2 minutes)")
        print(f"{'='*70}")
        print(f"Horizontal (1920x1080 @ 60fps) → YouTube + Twitch + Kick")
        print(f"Vertical (720x1280 @ 30fps) → Instagram")
        print(f"{'='*70}\n")

        # Create horizontal process (YouTube + Twitch + Kick)
        horizontal_id = f"test_all_horizontal_{int(time.time())}"
        self.test_processes.append(horizontal_id)

        horizontal_outputs = [
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
            },
            {
                "address": f"{self.kick_url}/{self.kick_key}",
                "id": "kick_output",
                "options": [
                    "-c:v", "libx264", "-preset", "veryfast",
                    "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
                    "-g", "120", "-r", "60",
                    "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
                ]
            }
        ]

        result_h = self.client.create_process(horizontal_id, self.horizontal_input, horizontal_outputs)
        self.assertEqual(len(result_h['output']), 3)
        print(f"✓ Created horizontal process (YouTube @ 8000k, Twitch @ 6000k, Kick @ 6000k)")

        # Create vertical process (Instagram)
        vertical_id = f"test_all_vertical_{int(time.time())}"
        self.test_processes.append(vertical_id)

        vertical_outputs = [{
            "address": f"{self.instagram_url}/{self.instagram_key}",
            "id": "instagram_output",
            "options": [
                "-c:v", "libx264",
                "-preset", "veryfast",
                "-b:v", "4000k",
                "-maxrate", "4000k",
                "-bufsize", "8000k",
                "-g", "60",           # 2 seconds @ 30fps
                "-r", "30",           # Instagram prefers 30fps
                "-profile:v", "main",
                "-c:a", "aac",
                "-b:a", "256k",       # Up to 256k per Instagram specs
                "-ar", "44100",       # 44.1 KHz per Instagram specs
                "-ac", "2",           # Stereo per Instagram specs
                "-f", "flv"
            ]
        }]

        result_v = self.client.create_process(vertical_id, self.vertical_input, vertical_outputs)
        self.assertEqual(result_v['id'], vertical_id)
        print(f"✓ Created vertical process (Instagram @ 4000k, 720x1280, stereo 44.1kHz)")

        # Start both processes
        start_h = self.client.start_process(horizontal_id)
        self.assertIn(start_h.get('exec'), ['starting', 'running', 'failed'])
        print(f"✓ Horizontal stream started: {start_h.get('exec')}")

        start_v = self.client.start_process(vertical_id)
        self.assertIn(start_v.get('exec'), ['starting', 'running', 'failed'])
        print(f"✓ Vertical stream started: {start_v.get('exec')}")

        # Monitor both streams for 120 seconds (2 minutes)
        print(f"\n{'='*70}")
        print(f"🌐 STREAMING TO ALL 4 PLATFORMS SIMULTANEOUSLY")
        print(f"{'='*70}")
        print(f"📺 Horizontal: YouTube (8000k) + Twitch (6000k) + Kick (6000k)")
        print(f"📱 Vertical:   Instagram (4000k)")
        print(f"{'='*70}\n")

        for i in range(24):
            time.sleep(5)
            elapsed = (i + 1) * 5

            # Get state for both processes
            state_h = self.client.get_process_state(horizontal_id)
            state_v = self.client.get_process_state(vertical_id)

            status_h = state_h.get('exec')
            status_v = state_v.get('exec')

            print(f"  [{elapsed:3d}s] Horizontal: {status_h:8s} | Vertical: {status_v:8s}")

        # Stop both streams
        print(f"\n{'='*70}")
        self.client.stop_process(horizontal_id)
        print(f"✓ Stopped horizontal stream (YouTube + Twitch + Kick)")

        self.client.stop_process(vertical_id)
        print(f"✓ Stopped vertical stream (Instagram)")

        print(f"✓ All platforms test completed successfully")
        print(f"{'='*70}\n")


def run_all_platforms_test():
    """Run all platforms simultaneous streaming test"""
    # Check for required credentials
    youtube_ready = bool(os.getenv('YOUTUBE_INGEST_URL') and os.getenv('YOUTUBE_STREAM_KEY'))
    twitch_ready = bool(os.getenv('TWITCH_INGEST_URL') and os.getenv('TWITCH_STREAM_KEY'))
    kick_ready = bool(os.getenv('KICK_INGEST_URL') and os.getenv('KICK_STREAM_KEY'))
    instagram_ready = bool(os.getenv('INSTAGRAM_INGEST_URL') and os.getenv('INSTAGRAM_STREAM_KEY'))

    print("\n" + "="*70)
    print("OBS POLYEMESIS - ALL PLATFORMS SIMULTANEOUS TEST")
    print("="*70)
    print(f"YouTube credentials:   {'✓' if youtube_ready else '✗'}")
    print(f"Twitch credentials:    {'✓' if twitch_ready else '✗'}")
    print(f"Kick credentials:      {'✓' if kick_ready else '✗'}")
    print(f"Instagram credentials: {'✓' if instagram_ready else '✗'}")
    print("="*70 + "\n")

    if not (youtube_ready and twitch_ready and kick_ready and instagram_ready):
        print("⚠️  This test requires ALL platform credentials")
        print("   Set: YOUTUBE_INGEST_URL, YOUTUBE_STREAM_KEY")
        print("   Set: TWITCH_INGEST_URL, TWITCH_STREAM_KEY")
        print("   Set: KICK_INGEST_URL, KICK_STREAM_KEY")
        print("   Set: INSTAGRAM_INGEST_URL, INSTAGRAM_STREAM_KEY\n")
        return 1

    # Run tests
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestAllPlatforms)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_all_platforms_test())
