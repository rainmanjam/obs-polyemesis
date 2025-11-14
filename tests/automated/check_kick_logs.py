#!/usr/bin/env python3
"""Check restreamer logs for Kick streaming errors"""

import os
import time
from test_polyemesis import RestreamerClient

def check_kick_logs():
    client = RestreamerClient(
        os.getenv('RESTREAMER_URL', 'http://localhost:8080'),
        os.getenv('RESTREAMER_USER', 'admin'),
        os.getenv('RESTREAMER_PASS', 'admin')
    )

    kick_url = os.getenv('KICK_INGEST_URL')
    kick_key = os.getenv('KICK_STREAM_KEY')
    horizontal_input = 'rtmp://localhost/live/obs_horizontal'

    process_id = f"test_kick_debug_{int(time.time())}"

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
        print("\n=== Creating Kick process ===")
        result = client.create_process(process_id, horizontal_input, outputs)
        print(f"Created: {result.get('id')}")

        print("\n=== Starting process ===")
        start_result = client.start_process(process_id)
        print(f"Start status: {start_result.get('exec')}")

        print("\n=== Waiting 10 seconds for error to occur ===")
        time.sleep(10)

        print("\n=== Getting process report (full logs) ===")
        report = client.get_process_report(process_id)

        print(f"\nProcess ID: {report.get('id')}")
        print(f"Created: {report.get('created_at')}")
        print(f"State: {report.get('state', {})}")

        print(f"\n--- Logs ---")
        logs = report.get('log', [])
        if logs:
            for log_entry in logs[-20:]:  # Last 20 log entries
                print(log_entry)
        else:
            print("No logs available")

        print(f"\n--- Progress Data ---")
        progress = report.get('progress', {})
        print(f"Input: {progress.get('input', {})}")
        print(f"Output: {progress.get('output', [])}")

        # Cleanup
        client.stop_process(process_id)
        time.sleep(1)
        client.delete_process(process_id)

    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        try:
            client.stop_process(process_id)
            client.delete_process(process_id)
        except Exception:
            pass

if __name__ == '__main__':
    check_kick_logs()
