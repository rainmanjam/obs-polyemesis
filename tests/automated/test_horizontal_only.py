#!/usr/bin/env python3
"""
OBS Polyemesis - Horizontal Platforms Test (YouTube + Twitch + Kick)
Tests streaming to YouTube, Twitch, and Kick simultaneously for 2 minutes
"""

import os
import time
from test_polyemesis import RestreamerClient

def test_horizontal_platforms():
    """Test YouTube + Twitch + Kick for 2 minutes"""

    # Setup client
    client = RestreamerClient(
        os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
        os.getenv('RESTREAMER_USER', 'admin'),
        os.getenv('RESTREAMER_PASS', 'admin')
    )

    # Get credentials
    twitch_url = os.getenv('TWITCH_INGEST_URL')
    twitch_key = os.getenv('TWITCH_STREAM_KEY')
    youtube_url = os.getenv('YOUTUBE_INGEST_URL')
    youtube_key = os.getenv('YOUTUBE_STREAM_KEY')
    kick_url = os.getenv('KICK_INGEST_URL')
    kick_key = os.getenv('KICK_STREAM_KEY')

    horizontal_input = os.getenv('OBS_RTMP_URL_HORIZONTAL', 'rtmp://localhost/live/obs_horizontal')

    # Check credentials
    if not all([youtube_url, youtube_key, twitch_url, twitch_key, kick_url, kick_key]):
        print("❌ Missing credentials. Need YouTube, Twitch, and Kick credentials.")
        return 1

    print(f"\n{'='*70}")
    print(f"HORIZONTAL PLATFORMS TEST (2 minutes)")
    print(f"{'='*70}")
    print(f"YouTube @ 8000k + Twitch @ 6000k + Kick @ 6000k")
    print(f"1920x1080 @ 60fps")
    print(f"{'='*70}\n")

    # Create horizontal process
    process_id = f"test_horizontal_{int(time.time())}"

    outputs = [
        {
            "address": f"{youtube_url}/{youtube_key}",
            "id": "youtube_output",
            "options": [
                "-c:v", "libx264", "-preset", "veryfast",
                "-b:v", "8000k", "-maxrate", "8000k", "-bufsize", "16000k",
                "-g", "120", "-r", "60",
                "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
            ]
        },
        {
            "address": f"{twitch_url}/{twitch_key}",
            "id": "twitch_output",
            "options": [
                "-c:v", "libx264", "-preset", "veryfast",
                "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
                "-g", "120", "-r", "60",
                "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
            ]
        },
        {
            "address": f"{kick_url}/{kick_key}",
            "id": "kick_output",
            "options": [
                "-c:v", "libx264", "-preset", "veryfast",
                "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
                "-g", "120", "-r", "60",
                "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
            ]
        }
    ]

    try:
        # Create process
        result = client.create_process(process_id, horizontal_input, outputs)
        print(f"✓ Created process with {len(result['output'])} outputs")

        # Start process
        start_result = client.start_process(process_id)
        print(f"✓ Started stream: {start_result.get('exec')}\n")

        # Monitor for 2 minutes
        print(f"{'='*70}")
        print(f"🌐 STREAMING TO YOUTUBE + TWITCH + KICK")
        print(f"{'='*70}\n")

        for i in range(24):  # 24 checks × 5 seconds = 2 minutes
            time.sleep(5)
            elapsed = (i + 1) * 5

            state = client.get_process_state(process_id)
            status = state.get('exec', 'unknown')

            # Get individual output states if available
            progress = state.get('progress', {})

            print(f"  [{elapsed:3d}s] Status: {status:8s}", end='')

            if 'output' in progress:
                outputs_info = []
                for idx, output in enumerate(progress['output']):
                    state_val = output.get('state', 'unknown')
                    if idx == 0:  # YouTube
                        outputs_info.append(f"YT:{state_val}")
                    elif idx == 1:  # Twitch
                        outputs_info.append(f"TW:{state_val}")
                    elif idx == 2:  # Kick
                        outputs_info.append(f"KK:{state_val}")

                print(f" | {' | '.join(outputs_info)}")
            else:
                print()

        # Stop stream
        print(f"\n{'='*70}")
        client.stop_process(process_id)
        print(f"✓ Stopped stream")

        time.sleep(2)
        client.delete_process(process_id)
        print(f"✓ Cleaned up process")
        print(f"✓ Test completed successfully")
        print(f"{'='*70}\n")

        return 0

    except Exception as e:
        print(f"❌ Error: {e}")
        try:
            client.stop_process(process_id)
            client.delete_process(process_id)
        except:
            pass
        return 1

if __name__ == '__main__':
    import sys
    sys.exit(test_horizontal_platforms())
