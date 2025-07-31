from flask import Flask, request, jsonify, send_file
from flask_cors import CORS
import os
import tempfile
import shutil
import subprocess

app = Flask(__name__)
CORS(app)

@app.route('/compile', methods=['POST'])
def compile_firmware():
    # Expecting files: main, font, html and form fields: board, platform
    if not all(k in request.files for k in ('main', 'font', 'html')):
        print("Missing files in request")
        return jsonify({'error': 'Missing one or more required files'}), 400
    if not all(k in request.form for k in ('board', 'platform')):
        print("Missing board or platform in request")
        return jsonify({'error': 'Missing board or platform'}), 400

    board = request.form['board']
    platform = request.form['platform']

    src_dir = "./build/src"
    build_dir = "./build"

    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    os.makedirs(build_dir, exist_ok=True)

    if os.path.exists(src_dir):
        shutil.rmtree(src_dir)
    os.makedirs(src_dir, exist_ok=True)

    try:
        # Save uploaded files in src/
        main_path = os.path.join(src_dir, 'main.cpp')
        font_path = os.path.join(src_dir, 'font5x7.h')
        html_path = os.path.join(src_dir, 'html.h')
        request.files['main'].save(main_path)
        request.files['font'].save(font_path)
        request.files['html'].save(html_path)

        # Read and update platformio.ini template
        with open('platformio.ini', 'r') as f:
            ini_content = f.read()
        ini_content = ini_content.replace('{BOARD}', board).replace('{PLATFORM}', platform)
        with open(os.path.join(build_dir, 'platformio.ini'), 'w') as f:
            f.write(ini_content)

        # Run the build
        build_cmd = [r'.\WPy64-312101\python\python.exe', '-m', 'platformio', 'run', '-d', build_dir]
        result = subprocess.run(build_cmd, capture_output=True, text=True)
        print(result.stdout)
        print(result.stderr)
        if result.returncode != 0:
            print("Build failed")
            return jsonify({'error': 'Build failed', 'output': result.stdout + result.stderr}), 500

        build_dir = os.path.join(os.getcwd(), 'build')
        bin_path = os.path.join(build_dir, '.pio', 'build', board, 'firmware.bin')
        print(bin_path)
        if not os.path.exists(bin_path):
            print("Build succeeded but .bin not found")
            return jsonify({'error': 'Build succeeded but .bin not found'}), 500

        return send_file(bin_path, as_attachment=True, download_name='firmware.bin')
    finally:
        print("Done!")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)