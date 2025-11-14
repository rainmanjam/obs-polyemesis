#!/usr/bin/env python3
"""
OBS Polyemesis - Kick Only Test
Tests streaming to Kick for 2 minutes
"""

import os
import time
from test_polyemesis import RestreamerClient

def test_kick_only():
    """Test Kick streaming for 2 minutes"""

    # Setup client
    client = RestreamerClient(
        os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
        os.getenv('RESTREAMER_USER', 'admin'),
        os.getenv('RESTREAMER_PASS', 'admin')
    )

    # Get credentials
    kick_url = os.getenv('KICK_INGEST_URL')
    kick_key = os.getenv('KICK_STREAM_KEY')
    horizontal_input = os.getenv('OBS_RTMP_URL_HORIZONTAL', 'rtmp://localhost/live/obs_horizontal')

    # Check credentials
    if not (kick_url and kick_key):
        print("❌ Missing Kick credentials")
        return 1

    print(f"\n{'='*70}")
    print(f"KICK ONLY TEST (2 minutes)")
    print(f"{'='*70}")
    print(f"Kick @ 6000k | 1920x1080 @ 60fps")
    print(f"RTMPS: {kick_url}")
    print(f"{'='*70}\n")

    # Create process
    process_id = f"test_kick_{int(time.time())}"

    outputs = [{
        "address": f"{kick_url}/{kick_key}",
        "id": "kick_output",
        "options": [
            "-c:v", "libx264", "-preset", "veryfast",
            "-b:v", "6000k", "-maxrate", "6000k", "-bufsize", "12000k",
            "-g", "120", "-r", "60",
            "-c:a", "aac", "-b:a", "160k", "-ar", "48000", "-f", "flv"
        ]
    }]

    try:
        # Create process
        result = client.create_process(process_id, horizontal_input, outputs)
        print(f"✓ Created process with {len(result['output'])} output")
        print(f"  Output address: {result['output'][0].get('address', 'N/A')}")

        # Start process
        start_result = client.start_process(process_id)
        print(f"✓ Started stream: {start_result.get('exec')}\n")

        # Monitor for 2 minutes
        print(f"{'='*70}")
        print(f"🌐 STREAMING TO KICK")
        print(f"{'='*70}\n")

        for i in range(24):  # 24 checks × 5 seconds = 2 minutes
            time.sleep(5)
            elapsed = (i + 1) * 5

            state = client.get_process_state(process_id)
            status = state.get('exec', 'unknown')

            # Get detailed output state
            progress = state.get('progress', {})
            output_state = 'unknown'

            if 'output' in progress and len(progress['output']) > 0:
                output_info = progress['output'][0]
                output_state = output_info.get('state', 'unknown')

                print(f"  [{elapsed:3d}s] Process: {status:8s} | Output: {output_state:8s}", end='')

                # Show additional debug info
                if 'packet' in output_info:
                    packets = output_info['packet']
                    print(f" | Packets: {packets}", end='')
                if 'time' in output_info:
                    stream_time = output_info['time']
                    print(f" | Time: {stream_time:.1f}s", end='')

                print()
            else:
                print(f"  [{elapsed:3d}s] Process: {status:8s} | Output: {output_state}")

        # Stop stream
        print(f"\n{'='*70}")
        client.stop_process(process_id)
        print(f"✓ Stopped stream")

        time.sleep(2)
        client.delete_process(process_id)
        print(f"✓ Cleaned up process")
        print(f"✓ Test completed")
        print(f"{'='*70}\n")

        return 0

    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        try:
            client.stop_process(process_id)
            client.delete_process(process_id)
        except Exception:
            # Ignore cleanup errors - process may already be stopped
            pass
        return 1

if __name__ == '__main__':
    import sys
    sys.exit(test_kick_only())
