const char serverIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
    <html lang='en' class=''>
    <head>
        <meta charset='UTF-8'>
        <title>Matrix Clock Control Panel</title>
        <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
        <script src='https://cdn.tailwindcss.com'></script>
        <!-- MDI CDN -->
        <link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/@mdi/font@7.4.47/css/materialdesignicons.min.css'>
        <script>
            tailwind.config = { darkMode: 'class' }
        </script>
        <!-- Add in your <head> -->
        <!-- Monaco Editor CDN -->
        <script src='https://cdn.jsdelivr.net/npm/monaco-editor@0.45.0/min/vs/loader.js'></script>
    </head>
    <body class='bg-gray-100 dark:bg-gray-900 min-h-screen transition-colors duration-300'>
    <!-- Top Bar -->
    <div class='flex items-center justify-between px-6 py-4 bg-white dark:bg-gray-800 shadow'>
        <div class='flex items-center gap-6'>
            <div class='font-bold text-blue-600 dark:text-blue-300 text-lg flex items-center gap-1 select-none cursor-pointer'>
                <i class='mdi mdi-clock-outline'></i> Matrix Clock
            </div>
            <button type='button' id='nav-main-link' class='nav-link text-gray-700 dark:text-gray-200 flex items-center gap-1 focus:outline-none bg-transparent'
                    onclick='location.hash='#/''>
                <i class='mdi mdi-cog-outline'></i> Main
            </button>
            <button type='button' id='nav-updater-link' class='nav-link text-gray-700 dark:text-gray-200 flex items-center gap-1 focus:outline-none bg-transparent'
                    onclick='location.hash='#/updater''>
                <i class='mdi mdi-update'></i> Updater
            </button>
            <button type='button' id='nav-editor-link' class='nav-link text-gray-700 dark:text-gray-200 flex items-center gap-1 focus:outline-none bg-transparent'
                    onclick='location.hash='#/editor''>
                <i class='mdi mdi-code-tags'></i> Live Editor
            </button>
        </div>
        <div class='flex items-center gap-4'>
            <!-- Status Box with WiFi Icon -->
            <div id='statusBox' class='flex items-center gap-2 px-3 py-1 rounded bg-gray-200 dark:bg-gray-700 text-sm cursor-pointer'>
                <i id='wifiIcon' class='mdi mdi-wifi text-gray-400 cursor-pointer'></i>
                <span id='ipAddress' class='ml-1 dark:text-white cursor-pointer'>IP: --</span>
            </div>
            <!-- Dark Mode Switch -->
            <label for='darkModeSwitch' class='flex items-center cursor-pointer ml-4'>
                <div class='relative'>
                    <input type='checkbox' id='darkModeSwitch' class='sr-only'>
                    <div class='block bg-gray-300 dark:bg-gray-700 w-12 h-7 rounded-full transition'></div>
                    <div class='dot absolute left-1 top-1 bg-white w-5 h-5 rounded-full transition transform dark:translate-x-5 flex items-center justify-center'>
                        <i class='mdi mdi-weather-night text-gray-700 dark:text-blue-800 text-lg'></i>
                    </div>
                </div>
            </label>
        </div>
    </div>
    <!-- Main Content -->
    <div id='pageContent' class='p-8'></div>

    <script>
        // --- Dark mode persistence ---
        function setDarkMode(enabled) {
            $('html').toggleClass('dark', enabled);
            $('#darkModeSwitch').prop('checked', enabled);
            localStorage.setItem('darkMode', enabled ? '1' : '0');
        }

        function setActiveTab(hash) {
            // Remove active classes from all
            $('.nav-link').removeClass('border-b-2 border-blue-600 dark:border-blue-300 text-blue-600 dark:text-blue-300 font-semibold');
            // Add to the current
            if (hash === '#/updater') {
                $('#nav-updater-link').addClass('border-b-2 border-blue-600 dark:border-blue-300 text-blue-600 dark:text-blue-300 font-semibold');
            } else if (hash === '#/editor') {
                $('#nav-editor-link').addClass('border-b-2 border-blue-600 dark:border-blue-300 text-blue-600 dark:text-blue-300 font-semibold');
            } else {
                $('#nav-main-link').addClass('border-b-2 border-blue-600 dark:border-blue-300 text-blue-600 dark:text-blue-300 font-semibold');
            }
        }
        // --- Simple Router ---
        function loadPage(hash) {
            setActiveTab(hash);
            let previewInterval = null;
            let previewLoop = false;

            if (hash === '#/updater') {
                $('#pageContent').html(`
                <div class='flex justify-center items-start w-full'>
                    <div class='bg-white dark:bg-gray-800 shadow-lg rounded-lg p-8 w-full max-w-md transition-colors duration-300'>
                        <h1 class='text-2xl font-bold mb-6 text-center text-gray-800 dark:text-gray-100 flex items-center justify-center gap-2'>
                            <i class='mdi mdi-update text-blue-600 dark:text-blue-300'></i>
                            Matrix Clock Manual Update
                        </h1>
                        <form method='POST' action='#' enctype='multipart/form-data' id='upload_form' class='flex flex-col gap-4'>
                            <label class='flex items-center gap-2'>
                                <i class='mdi mdi-file-upload-outline text-blue-600 dark:text-blue-300'></i>
                                <input type='file' name='update' id='fileInput' class='block w-full text-sm text-gray-700 dark:text-gray-200 file:mr-4 file:py-2 file:px-4 file:rounded file:border-0 file:text-sm file:font-semibold file:bg-blue-600 dark:file:bg-blue-600 file:text-white dark:file:text-white hover:file:bg-blue-700 dark:hover:file:bg-blue-700' required>
                            </label>
                            <button type='submit' class='bg-blue-600 hover:bg-blue-700 text-white font-semibold py-2 rounded transition flex items-center justify-center gap-2'>
                                <i class='mdi mdi-upload'></i> Update
                            </button>
                        </form>
                        <div class='mt-4'>
                            <div class='w-full bg-gray-200 dark:bg-gray-700 rounded-full h-4'>
                                <div id='progressBar' class='bg-blue-600 dark:bg-blue-400 h-4 rounded-full transition-all duration-300' style='width: 0%'></div>
                            </div>
                            <div id='prg' class='mt-2 text-center font-semibold text-blue-700 dark:text-blue-300'>progress: 0%</div>
                            <div id='doneMsg' class='mt-2 text-center font-bold text-green-600 dark:text-green-400 hidden flex items-center justify-center gap-1'>
                                <i class='mdi mdi-check-circle-outline'></i> Done!
                            </div>
                            <div id='uploadMsg' class='mt-2 text-center font-semibold text-green-600 dark:text-green-400 hidden'></div>
                            <div id='errorMsg' class='mt-2 text-center font-semibold text-red-600 dark:text-red-400 hidden flex items-center justify-center gap-1'>
                                <i class='mdi mdi-alert-circle-outline'></i>
                            </div>
                        </div>
                    </div>
                </div>
                `);
                // --- Upload form logic ---
                $('#upload_form').submit(function(e){
                    e.preventDefault();
                    $('#doneMsg, #uploadMsg, #errorMsg').addClass('hidden');
                    $('#prg').removeClass('hidden').text('progress: 0%');
                    $('#progressBar').css('width', '0%');
                    var form = $('#upload_form')[0];
                    var data = new FormData(form);
                    $.ajax({
                        url: '/update',
                        type: 'POST',
                        data: data,
                        contentType: false,
                        processData: false,
                        xhr: function() {
                            var xhr = new window.XMLHttpRequest();
                            xhr.upload.addEventListener('progress', function(evt) {
                                if (evt.lengthComputable) {
                                    var per = evt.loaded / evt.total;
                                    var percent = Math.round(per * 100);
                                    $('#progressBar').css('width', percent + '%');
                                    $('#prg').text('progress: ' + percent + '%');
                                }
                            }, false);
                            return xhr;
                        },
                        success: function(d, s) {
                            console.log('AJAX success:', { data: d, status: s });
                            $('#progressBar').css('width', '0%');
                            $('#prg').addClass('hidden');
                            $('#doneMsg').removeClass('hidden');
                            $('#uploadMsg').removeClass('hidden').text('Upload successful! The board will reboot.');
                            $('#fileInput').val('');
                            setTimeout(function() {
                                $('#doneMsg').addClass('hidden');
                                setTimeout(function() {
                                    location.reload();
                                }, 2500);
                            }, 2000);
                        },
                        error: function(a, b, c) {
                            console.log('AJAX error:', { jqXHR: a, textStatus: b, errorThrown: c });
                            $('#progressBar').css('width', '0%');
                            $('#prg').addClass('hidden');
                            $('#doneMsg').removeClass('hidden');
                            $('#uploadMsg').removeClass('hidden').text('Upload successful! The board will reboot.');
                            $('#fileInput').val('');
                            setTimeout(function() {
                                $('#doneMsg').addClass('hidden');
                                setTimeout(function() {
                                    location.reload();
                                }, 2500);
                            }, 2000);
                        }
                    });
                });
            } else if (hash === '#/editor') {
                const files = [
                    { name: 'main.cpp', url: 'https://raw.githubusercontent.com/vaggos-thanos/LED_clock/main/LED_clock_esp32/LED_clock_esp32.ino' },
                    { name: 'font5x7.h', url: 'https://raw.githubusercontent.com/vaggos-thanos/LED_clock/main/LED_clock_esp32/font5x7.h' },
                    { name: 'html.h', url: 'https://raw.githubusercontent.com/vaggos-thanos/LED_clock/main/LED_clock_esp32/html.h' },
                ];

                // Update the main content div in your HTML
                $('#pageContent').removeClass('max-w-2xl');

                // In your editor section, update the container HTML
                $('#pageContent').html(`
                    <div class='h-[calc(100vh-10rem)] w-full'>
                        <div id='editor-tabs' class='flex gap-2 mb-2 border-b border-gray-300 dark:border-gray-600'></div>

                        <div id='monaco-container' class='bg-gray-800 h-[calc(100%-6rem)] w-full' style='border:1px solid #444;border-radius:6px;'></div>
                        <div class='flex gap-6 mb-4'>
                            <div class='mt-4 flex gap-4'>
                                <button id='buildFirmwareBtn' class='bg-green-600 hover:bg-green-700 text-white font-semibold py-2 px-4 rounded flex items-center gap-2 transition-colors'>
                                    <i class='mdi mdi-hammer-wrench'></i> Build & Upload Firmware
                                </button>
                                <button id='revertBtn' disabled class='bg-red-600 hover:bg-red-700 text-white font-semibold py-2 px-4 rounded flex items-center gap-2 transition-colors disabled:opacity-50 disabled:cursor-not-allowed'>
                                    <i class='mdi mdi-undo'></i> Revert Changes
                                </button>
                                <span id='buildStatus' class='ml-4 text-sm dark:text-white'></span>

                            </div>
                          <!-- Board Selection -->
                          <div class='flex-1'>
                            <label class='block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1'>Board</label>
                            <div class='flex gap-2'>
                              <select id='boardSelect' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2 w-full'>
                                <option value='esp32dev'>esp32dev</option>
                                <option value='esp32doit-devkit-v1' selected>esp32doit-devkit-v1</option>
                                <option value=''>Custom...</option>
                              </select>
                              <input id='boardInput' type='text' placeholder='Custom board' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2 w-full' style='display:none;'>
                            </div>
                          </div>
                          <!-- Platform Selection -->
                          <div class='flex-1'>
                            <label class='block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1'>Platform</label>
                            <div class='flex gap-2'>
                              <select id='platformSelect' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2 w-full'>
                                <option value='espressif32'>espressif32</option>
                                <option value='espressif8266'>espressif8266</option>
                                <option value=''>Custom...</option>
                              </select>
                              <input id='platformInput' type='text' placeholder='Custom platform' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2 w-full' style='display:none;'>
                            </div>
                          </div>
                        </div>
                    </div>
                `);

                // Update the page content padding in your HTML

                let currentFile = files[0].name;
                let originalContents = {};
                let modifiedContents = {};

                require.config({ paths: { 'vs': 'https://cdn.jsdelivr.net/npm/monaco-editor@0.45.0/min/vs' }});
                require(['vs/editor/editor.main'], function() {
                    Promise.all(files.map(f => fetch(f.url).then(r => r.text()))).then(contents => {
                        // Store original contents
                        files.forEach((f, i) => {
                            if (f.name === 'html.h') {
                                contents[i] = getHTMLFromHeader(contents[i]);
                                f.name = 'index.html'; // Update file name for HTML content
                            }
                            originalContents[f.name] = contents[i];
                            modifiedContents[f.name] = contents[i];
                        });

                        // Create tabs
                        files.forEach((f, i) => {
                            $('#editor-tabs').append(`
                                <button class='tab-btn px-4 py-2 text-gray-700 dark:text-gray-300
                                    ${i === 0 ? 'bg-gray-100 dark:bg-gray-700 border-b-2 border-blue-600 dark:border-blue-400' : 'hover:bg-gray-100 dark:hover:bg-gray-700'}
                                    rounded-t transition-colors duration-200 focus:outline-none'
                                    data-file='${f.name}'>
                                    ${f.name}
                                </button>
                            `);
                        });

                        $('#editor-tabs').append(`
                          <div class='ml-auto flex items-center gap-2'>
                            <label for='versionSelect' class='text-sm text-gray-600 dark:text-gray-300'>Version:</label>
                            <select id='versionSelect' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-2 py-1'>
                              <option value='published'>Published</option>
                              <option value='custom'>Custom</option>
                            </select>
                          </div>
                        `);

                        // Create editor container
                        const editorContainer = document.createElement('div');
                        editorContainer.style = 'width:100%;height:100%;';
                        document.getElementById('monaco-container').appendChild(editorContainer);

                        // Create Monaco editor instance
                        const editor = monaco.editor.create(editorContainer, {
                            value: contents[0],
                            language: 'cpp',
                            theme: 'vs-dark',
                            fontSize: 14,
                            minimap: { enabled: true },
                            automaticLayout: true,
                            scrollBeyondLastLine: false,
                            renderLineHighlight: 'all',
                            scrollbar: {
                                useShadows: false,
                                verticalScrollbarSize: 10,
                                horizontalScrollbarSize: 10
                            }
                        });

                        // Create git decorations provider
                        const decorationsCollection = editor.createDecorationsCollection();

                        // Update the updateGitDecorations function and add line color styles
                        function updateGitDecorations() {
                            const originalLines = originalContents[currentFile].split('\n');
                            const modifiedLines = editor.getValue().split('\n');
                            const decorations = [];

                            modifiedLines.forEach((line, lineNumber) => {
                                if (lineNumber >= originalLines.length) {
                                    // Added line - Green
                                    decorations.push({
                                        range: new monaco.Range(lineNumber + 1, 1, lineNumber + 1, 1),
                                        options: {
                                            isWholeLine: true,
                                            linesDecorationsClassName: 'gitAddedLine',
                                            marginClassName: 'gitAddedMargin'
                                        }
                                    });
                                } else if (line !== originalLines[lineNumber]) {
                                    // Modified line - Yellow
                                    decorations.push({
                                        range: new monaco.Range(lineNumber + 1, 1, lineNumber + 1, 1),
                                        options: {
                                            isWholeLine: true,
                                            linesDecorationsClassName: 'gitModifiedLine',
                                            marginClassName: 'gitModifiedMargin'
                                        }
                                    });
                                }
                            });

                            // Check for deleted lines - Red
                            if (originalLines.length > modifiedLines.length) {
                                for (let i = modifiedLines.length; i < originalLines.length; i++) {
                                    decorations.push({
                                        range: new monaco.Range(i + 1, 1, i + 1, 1),
                                        options: {
                                            isWholeLine: true,
                                            linesDecorationsClassName: 'gitDeletedLine',
                                            marginClassName: 'gitDeletedMargin'
                                        }
                                    });
                                }
                            }

                            decorationsCollection.set(decorations);
                        }

                        // Update the style element creation
                        const style = document.createElement('style');
                        style.textContent = `
                            .gitAddedMargin {
                                border-left: 3px solid #28a745 !important;
                            }
                            .gitAddedLine {
                                background: rgba(40, 167, 69, 0.1);
                            }
                            .gitModifiedMargin {
                                border-left: 3px solid #ffc107 !important;
                            }
                            .gitModifiedLine {
                                background: rgba(255, 193, 7, 0.1);
                            }
                            .gitDeletedMargin {
                                border-left: 3px solid #dc3545 !important;
                            }
                            .gitDeletedLine {
                                background: rgba(220, 53, 69, 0.1);
                            }
                        `;
                        document.head.appendChild(style);

                        // Fix the revert changes button handler
                        $('#revertBtn').on('click', function() {
                            if (confirm('Are you sure you want to revert all changes in this file?')) {
                                editor.setValue(originalContents[currentFile]);
                                modifiedContents[currentFile] = originalContents[currentFile];
                                editor.getModel().setValue(originalContents[currentFile]); // Force update
                                updateGitDecorations();
                            }
                        });

                        // Track changes
                        // Update button enable/disable logic
                        function updateButtonStates() {
                            const hasChanges = modifiedContents[currentFile] !== originalContents[currentFile];
                            $('#updateCodeBtn').prop('disabled', !hasChanges);
                            $('#revertBtn').prop('disabled', !hasChanges);
                        }

                        // Track changes
                        editor.onDidChangeModelContent(() => {
                            modifiedContents[currentFile] = editor.getValue();
                            updateGitDecorations();
                            updateButtonStates();
                        });

                        // Update tab switching
                        $('.tab-btn').on('click', function() {
                            const fname = $(this).data('file');

                            // Save current content before switching
                            modifiedContents[currentFile] = editor.getValue();

                            // Update tab styling
                            $('.tab-btn').removeClass('bg-gray-100 dark:bg-gray-700 border-b-2 border-blue-600 dark:border-blue-400')
                                .addClass('hover:bg-gray-100 dark:hover:bg-gray-700');
                            $(this).addClass('bg-gray-100 dark:bg-gray-700 border-b-2 border-blue-600 dark:border-blue-400')
                                .removeClass('hover:bg-gray-100 dark:hover:bg-gray-700');

                            // Update editor content
                            currentFile = fname;
                            const lang = fname === 'index.html' ? 'html' : 'cpp';
                            monaco.editor.setModelLanguage(editor.getModel(), lang);
                            console.log('Switching to file:', fname, 'with language:', lang);
                            editor.getModel().setValue(modifiedContents[fname]);
                            updateGitDecorations();
                            updateButtonStates();
                        });


                        $('#versionSelect').on('change', function() {
                            const version = $(this).val();
                            if (version === 'custom') {
                                // Load from localStorage if exists
                                const saved = localStorage.getItem('customCode');
                                if (saved) {
                                    console.log('Loading custom code from localStorage:', saved);
                                    Object.assign(modifiedContents, JSON.parse(saved));
                                    editor.getModel().setValue(modifiedContents[currentFile]);
                                }

                                // Save current edits to localStorage
                                localStorage.setItem('customCode', JSON.stringify(modifiedContents));
                            } else {
                                // Load from GitHub (originalContents)
                                Object.assign(modifiedContents, originalContents);
                                editor.getModel().setValue(modifiedContents[currentFile]);
                            }
                            updateGitDecorations();
                            updateButtonStates();
                        });

                        // Save changes to localStorage when editing in 'Custom' mode
                        editor.onDidChangeModelContent(() => {
                            if ($('#versionSelect').val() === 'custom') {
                                modifiedContents[currentFile] = editor.getValue();
                                localStorage.setItem('customCode', JSON.stringify(modifiedContents));
                                console.log('Modified contents updated:', modifiedContents);
                                console.log('updated localStorage with custom code');
                            }

                            updateGitDecorations();
                            updateButtonStates();
                        });

                        let editorInstances = {};
                        files.forEach((f, i) => {
                            // Store the editor instance for each file (if you use multiple Monaco editors)
                            // In your setup, you have one editor and switch content, so track content per file:
                            editorInstances[f.name] = {
                                getValue: () => modifiedContents[f.name]
                            };
                        });

                        $('#buildFirmwareBtn').on('click', async function() {
                            $('#buildStatus').text('Building...');
                            // Collect code from editors (from modifiedContents)
                            const mainCode = editorInstances['main.cpp'].getValue();
                            const fontCode = editorInstances['font5x7.h'].getValue();
                            const htmlCode = getHeaderFromHTML(editorInstances['index.html'].getValue());

                            // Prepare form data
                            const formData = new FormData();
                            formData.append('main', new Blob([mainCode], {type: 'text/plain'}), 'main.cpp');
                            formData.append('font', new Blob([fontCode], {type: 'text/plain'}), 'font5x7.h');
                            formData.append('html', new Blob([htmlCode], {type: 'text/plain'}), 'html.h');
                            formData.append('board', getBoardValue());
                            formData.append('platform', getPlatformValue());


                            console.log('Sending form data:', {
                                main: mainCode,
                                font: fontCode,
                                html: htmlCode,
                                board: getBoardValue(),
                                platform: getPlatformValue()
                            });
                            try {
                                const response = await fetch('http://localhost:5000/compile', {
                                    method: 'POST',
                                    body: formData
                                });

                                if (!response.ok) {
                                    let err = 'No Clue MATE ;)';
                                    try { err = (await response.json()).details || (await response.json()).error; } catch {}
                                    console.error('Build error:', err);
                                    $('#buildStatus').text('Build failed: ' + err);
                                    return;
                                }

                                // Download the firmware.bin
                                const blob = await response.blob();
                                console.log('Build success:', blob);
                                $('#buildStatus').text('Build succeeded! Upload started.');
                            } catch (e) {
                                $('#buildStatus').text('Build error: ' + e);
                            }
                        });

                        // Show input if 'Custom...' is selected
                        $('#boardSelect').on('change', function() {
                            if ($(this).val() === '') {
                                $('#boardInput').show();
                            } else {
                                $('#boardInput').hide();
                            }
                        });

                        $('#platformSelect').on('change', function() {
                            if ($(this).val() === '') {
                                $('#platformInput').show();
                            } else {
                                $('#platformInput').hide();
                            }
                        });

                        // To get the selected value:
                        function getBoardValue() {
                            return $('#boardSelect').val() === '' ? $('#boardInput').val() : $('#boardSelect').val();
                        }
                        function getPlatformValue() {
                            return $('#platformSelect').val() === '' ? $('#platformInput').val() : $('#platformSelect').val();
                        }

                        function uploadFirmwareToESP32(firmwareBlob, esp32Ip) {
                            $('#doneMsg, #uploadMsg, #errorMsg').addClass('hidden');
                            $('#prg').removeClass('hidden').text('progress: 0%');
                            $('#progressBar').css('width', '0%');

                            const formData = new FormData();
                            formData.append('update', firmwareBlob, 'firmware.bin');

                            $.ajax({
                                url: `http://${esp32Ip}/update`,
                                type: 'POST',
                                data: formData,
                                contentType: false,
                                processData: false,
                                xhr: function() {
                                    var xhr = new window.XMLHttpRequest();
                                    xhr.upload.addEventListener('progress', function(evt) {
                                        if (evt.lengthComputable) {
                                            var per = evt.loaded / evt.total;
                                            var percent = Math.round(per * 100);
                                            $('#progressBar').css('width', percent + '%');
                                            $('#prg').text('progress: ' + percent + '%');
                                        }
                                    }, false);
                                    return xhr;
                                },
                                success: function() {
                                    $('#progressBar').css('width', '0%');
                                    $('#prg').addClass('hidden');
                                    $('#doneMsg').removeClass('hidden');
                                    $('#uploadMsg').removeClass('hidden').text('Firmware uploaded! ESP32 will reboot.');
                                },
                                error: function(a, b, c) {
                                    $('#progressBar').css('width', '0%');
                                    $('#prg').addClass('hidden');
                                    $('#errorMsg').removeClass('hidden').text('Firmware upload failed!');
                                }
                            });
                        }

                        function getHTMLFromHeader(headerCode) {
                            let text = headerCode.toString();
                            text = text.split('const char serverIndex[] PROGMEM =R\"rawliteral(')[1];
                            text = text.split(')rawliteral\";')
                            return text[0];
                        }

                        function getHeaderFromHTML(htmlCode) {
                            // Escape backslashes and double quotes for C++ string
                            // const escaped = htmlCode
                            //     .replace(/\\/g, '\\\\')
                            //     .replace(/'/g, '\\'');
                            return 'const char serverIndex[] PROGMEM = R"rawliteral(' + htmlCode + ')rawliteral\";';
                        }
                    });
                });
            } else {
                $('#pageContent').html(`
                    <div class='flex flex-col items-center w-full gap-8'>
                        <div class='bg-white dark:bg-gray-800 shadow-lg rounded-lg p-8 w-full max-w-3xl'>
                            <div class='flex items-center justify-between mb-6'>
                                <h2 class='text-xl font-bold flex items-center gap-2 text-gray-800 dark:text-gray-100'>
                                    <i class='mdi mdi-led-strip text-blue-600 dark:text-blue-300'></i>
                                    Matrix Display Preview
                                </h2>
                                <button id='liveToggle' class='border-2 border-red-500 text-red-500 hover:bg-red-500 hover:text-white px-4 py-2 rounded flex items-center justify-between transition-colors'>
                                    <span class='pr-3'>Live Preview</span>
                                    <span id='liveIndicator' class=' w-3 h-3 rounded-full bg-red-500 opacity-0'></span>
                                </button>
                            </div>

                            <!-- Matrix Display Container -->
                            <div class='bg-gray-900 p-4 rounded-lg'>
                                <div id='matrixDisplay' class='grid gap-1' style='grid-template-columns: repeat(32, 1fr);'>
                                    ${Array(8 * 32).fill().map(() => `
                                        <div class='led-pixel aspect-square rounded-full bg-gray-800 transition-colors duration-200'></div>
                                    `).join('')}
                                </div>
                            </div>

                            <!-- Display Controls -->
                            <div class='mt-6 grid grid-cols-2 gap-4'>
                                <div class='flex flex-col gap-2'>
                                    <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Brightness</label>
                                    <input type='range' min='0' max='100' value='50' class='w-full' id='brightnessControl'>
                                </div>
                                <div class='flex flex-col gap-2'>
                                    <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Refresh Rate</label>
                                    <select id='refreshRate' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                        <option value='1000'>1 second</option>
                                        <option value='500'>0.5 seconds</option>
                                        <option value='250'>0.25 seconds</option>
                                    </select>
                                </div>
                            </div>
                            <div class='mt-6 border-t pt-6'>
                                <div class='flex justify-between items-center mb-6'>
                                    <h2 class='text-xl font-bold flex items-center gap-2 text-gray-800 dark:text-gray-100'>
                                        <i class='mdi mdi-clock-settings text-blue-600 dark:text-blue-300'></i>
                                        Clock Settings
                                    </h2>
                                    <label class='flex items-center cursor-pointer gap-3'>
                                      <span class='text-gray-700 dark:text-gray-300 font-medium'>24/12 Hour</span>
                                      <div class='relative'>
                                        <input id='shortTimeSwitch' type='checkbox' class='sr-only peer'>
                                        <div class='w-12 h-7 rounded-full transition-colors duration-200
                                                    bg-gray-300 dark:bg-gray-700
                                                    peer-checked:bg-blue-500 dark:peer-checked:bg-blue-400'></div>
                                        <div class='absolute left-1 top-1 w-5 h-5 rounded-full
                                                    bg-white shadow-md flex items-center justify-center
                                                    transition-transform duration-200
                                                    peer-checked:translate-x-5'>
                                          <i id='shortTimeIcon' class='mdi mdi-sun text-lg transition-colors duration-200 text-yellow-500'></i>
                                        </div>
                                      </div>
                                    </label>
                                    <label class='flex items-center cursor-pointer gap-2'>
                                      <span class='text-gray-700 dark:text-gray-300 font-medium'>Preview</span>
                                      <div class='relative'>
                                        <input id='previewSwitch' type='checkbox' class='sr-only peer'>
                                        <div class='w-12 h-7 bg-gray-300 dark:bg-gray-700 rounded-full transition'></div>
                                        <div class='absolute left-1 top-1 bg-white w-5 h-5 rounded-full transition-transform duration-200 flex items-center justify-center peer-checked:translate-x-5'>
                                          <i class='mdi mdi-eye text-gray-700 dark:text-blue-800 text-lg'></i>
                                        </div>
                                      </div>
                                    </label>
                                </div>
                                <div class='grid grid-cols-2 md:grid-cols-3 gap-4'>
                                    <div class='flex flex-col gap-2'>
                                        <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Hour Color</label>
                                        <select id='hourColor' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                            <option value='1'>Red</option>
                                            <option value='2' selected>Green</option>
                                            <option value='3'>Yellow</option>
                                        </select>
                                    </div>
                                    <div class='flex flex-col gap-2'>
                                        <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Minute Color</label>
                                        <select id='minuteColor' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                            <option value='1'>Red</option>
                                            <option value='2' selected>Green</option>
                                            <option value='3'>Yellow</option>
                                        </select>
                                    </div>
                                    <div class='flex flex-col gap-2'>
                                        <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Seconds Color</label>
                                        <select id='secondsColor' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                            <option value='1'>Red</option>
                                            <option value='2' selected>Green</option>
                                            <option value='3'>Yellow</option>
                                        </select>
                                    </div>
                                    <div class='flex flex-col gap-2'>
                                        <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Colon Mode</label>
                                        <select id='colonMode' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                            <option value='0'>Disabled</option>
                                            <option value='1' selected>Static</option>
                                            <option value='2'>Blinking</option>
                                            <option value='3'>Static Dot</option>
                                            <option value='4'>Blinking Dot</option>
                                        </select>
                                    </div>
                                    <div class='flex flex-col gap-2'>
                                        <label class='text-sm font-medium text-gray-700 dark:text-gray-300'>Colon Color</label>
                                        <select id='colonColor' class='rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-700 text-gray-700 dark:text-gray-200 px-3 py-2'>
                                            <option value='1' selected>Red</option>
                                            <option value='2'>Green</option>
                                            <option value='3'>Yellow</option>
                                        </select>
                                    </div>
                                    <div class='flex flex-col justify-end'>
                                        <button id='saveSettings' class='bg-blue-600 hover:bg-blue-700 text-white font-semibold py-2 px-4 rounded flex items-center justify-center gap-2 transition-colors'>
                                            <i class='mdi mdi-content-save'></i>
                                            Save Settings
                                        </button>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                `);

                // Add styles for LED pixels
                const style = document.createElement('style');
                style.textContent = `
                    .led-pixel {
                        width: 100%;
                        transition: background-color 0.2s;
                    }
                    .led-pixel.active {
                        background-color: #00ff00;
                    }
                    #matrixDisplay {
                        min-width: 320px;
                        width: 100%;
                        max-width: 640px;
                        margin: 0 auto;
                    }
                    @keyframes pulse {
                        0% { opacity: 0.2; }
                        50% { opacity: 1; }
                        100% { opacity: 0.2; }
                    }

                    .live-active #liveIndicator {
                        opacity: 1;
                        animation: pulse 1.5s ease-in-out infinite;
                    }
                `;
                document.head.appendChild(style);

                // Matrix display update logic
                let refreshInterval;


                function updateMatrixDisplay() {
                    console.log('Fetching matrix state...');
                    $.get('/matrix-state', function(data) {
                        const pixels = document.querySelectorAll('.led-pixel');
                        const brightness = $('#brightnessControl').val() / 100;

                        data[0].forEach((row, y) => {
                            row.forEach((ledState, x) => {
                                const index = y * 32 + x;
                                const red = ledState.red ? 255 : 31;
                                const green = ledState.green ? 255 : 41;
                                const blue = red === 31 && green === 41 ? 55 : 0; // Default color if not lit
                                console.log(`Updating pixel at (${x}, ${y}) - Red: ${red}, Green: ${green}, Brightness: ${brightness}`);
                                const color = ledState ? `rgba(${red}, ${green}, ${blue}, ${brightness})` : 'rgb(31, 41, 55)';
                                pixels[index].style.backgroundColor = color;
                            });
                        });
                    })
                    .always(function() {
                    });
                }

                // Add this to your JavaScript logic
                let isLive = false;
                let liveInterval;
                let redBuffer = Array.from({ length: 8 }, () => Array(32).fill(0));
                let greenBuffer = Array.from({ length: 8 }, () => Array(32).fill(0));

                async function clearDisplay() {
                    redBuffer = Array.from({ length: 8 }, () => Array(32).fill(0));
                    greenBuffer = Array.from({ length: 8 }, () => Array(32).fill(0));
                    for (let row of document.querySelectorAll('.led-pixel')) {
                        row.style.backgroundColor = 'rgb(31, 41, 55)'; // Default color
                    }
                    console.log('Clearing display...');
                }

                // Initialize refresh interval
                function setRefreshRate() {
                    const rate = parseInt($('#refreshRate').val());
                    console.log(`Setting refresh rate to ${rate} ms`);
                    if (refreshInterval) clearInterval(refreshInterval);
                    if (liveInterval) {
                        clearInterval(liveInterval);
                        if (isLive) {
                            liveInterval = setInterval(updateMatrixDisplay, rate);
                        }
                    }
                }

                // Add this to handle settings updates
                function updateClockSettings() {
                    $.ajax({
                        url: '/update-settings',
                        method: 'POST',
                        contentType: 'application/json',
                        data: JSON.stringify({
                            hour_color: parseInt($('#hourColor').val()),
                            minute_color: parseInt($('#minuteColor').val()),
                            seconds_color: parseInt($('#secondsColor').val()),
                            colon_mode: parseInt($('#colonMode').val()),
                            colon_color: parseInt($('#colonColor').val())
                        }),
                        success: function(response) {
                            alert('Settings updated successfully!');
                        },
                        error: function() {
                            alert('Failed to update settings.');
                        }
                    });
                }


                function drawChar(x, y, char, color = 1) {
                    // Get character pattern from font array
                    const font5x7 = {
                        ' ': [0x00,0x00,0x00,0x00,0x00],
                        '!': [0x00,0x00,0x5F,0x00,0x00],
                        '\"': [0x00,0x07,0x00,0x07,0x00],
                        '#': [0x14,0x7F,0x14,0x7F,0x14],
                        '$': [0x24,0x2A,0x7F,0x2A,0x12],
                        '%': [0x23,0x13,0x08,0x64,0x62],
                        '&': [0x36,0x49,0x55,0x22,0x50],
                        '\'': [0x00,0x05,0x03,0x00,0x00],
                        '(': [0x00,0x1C,0x22,0x41,0x00],
                        ')': [0x00,0x41,0x22,0x1C,0x00],
                        '*': [0x14,0x08,0x3E,0x08,0x14],
                        '+': [0x08,0x08,0x3E,0x08,0x08],
                        ',': [0x00,0x50,0x30,0x00,0x00],
                        '-': [0x08,0x08,0x08,0x08,0x08],
                        '.': [0x40,0x00,0x00,0x00,0x00],
                        '/': [0x20,0x10,0x08,0x04,0x02],
                        '0': [0x3c,0x42,0x42,0x3c,0x00],
                        '1': [0x00,0x44,0x7e,0x40,0x00],
                        '2': [0x44,0x62,0x52,0x4c,0x00],
                        '3': [0x22,0x4a,0x56,0x22,0x00],
                        '4': [0x18,0x14,0x7e,0x10,0x00],
                        '5': [0x2e,0x4a,0x4a,0x32,0x00],
                        '6': [0x38,0x54,0x52,0x30,0x00],
                        '7': [0x02,0x72,0x0a,0x06,0x00],
                        '8': [0x24,0x5a,0x5a,0x24,0x00],
                        '9': [0x04,0x4a,0x2a,0x1c,0x00],
                        ':': [0x66,0x00,0x00,0x00,0x00],
                        ';': [0x00,0x56,0x36,0x00,0x00],
                        '<': [0x08,0x14,0x22,0x41,0x00],
                        '=': [0x14,0x14,0x14,0x14,0x14],
                        '>': [0x00,0x41,0x22,0x14,0x08],
                        '?': [0x02,0x01,0x51,0x09,0x06],
                        '@': [0x32,0x49,0x79,0x41,0x3E],
                        'A': [0x7c,0x12,0x12,0x7c,0x00],
                        'B': [0x7e,0x4a,0x4a,0x34,0x00],
                        'C': [0x3c,0x42,0x42,0x24,0x00],
                        'D': [0x7e,0x42,0x42,0x3c,0x00],
                        'E': [0x7e,0x4a,0x4a,0x42,0x00],
                        'F': [0x7e,0x0a,0x0a,0x02,0x00],
                        'G': [0x3c,0x42,0x52,0x34,0x00],
                        'H': [0x7e,0x08,0x08,0x7e,0x00],
                        'I': [0x00,0x42,0x7e,0x42,0x00],
                        'J': [0x20,0x40,0x42,0x3e,0x00],
                        'K': [0x7e,0x08,0x14,0x62,0x00],
                        'L': [0x7e,0x40,0x40,0x40,0x00],
                        'M': [0x7e,0x04,0x08,0x04,0x7e],
                        'N': [0x7e,0x04,0x08,0x10,0x7e],
                        'O': [0x3c,0x42,0x42,0x3c,0x00],
                        'P': [0x7e,0x12,0x12,0x0c,0x00],
                        'Q': [0x3c,0x42,0x52,0x6c,0x00],
                        'R': [0x7e,0x12,0x32,0x4c,0x00],
                        'S': [0x24,0x4a,0x4a,0x30,0x00],
                        'T': [0x02,0x02,0x7e,0x02,0x02],
                        'U': [0x3e,0x40,0x40,0x3e,0x00],
                        'V': [0x1e,0x20,0x40,0x20,0x1e],
                        'W': [0x3e,0x40,0x30,0x40,0x3e],
                        'X': [0x66,0x18,0x18,0x66,0x00],
                        'Y': [0x06,0x08,0x70,0x08,0x06],
                        'Z': [0x62,0x52,0x4a,0x46,0x00],
                        '[': [0x00,0x7E,0x42,0x00,0x00],
                        '\\': [0x02,0x04,0x08,0x10,0x20],
                        ']': [0x00,0x42,0x7E,0x00,0x00],
                        '^': [0x04,0x02,0x01,0x02,0x04],
                        '_': [0x40,0x40,0x40,0x40,0x40],
                        '`': [0x00,0x01,0x02,0x04,0x00],
                        'a': [0x20,0x54,0x54,0x78,0x00],
                        'b': [0x7E,0x48,0x48,0x30,0x00],
                        'c': [0x38,0x44,0x44,0x00,0x00],
                        'd': [0x30,0x48,0x48,0x7E,0x00],
                        'e': [0x38,0x54,0x54,0x18,0x00],
                        'f': [0x08,0x7C,0x0A,0x00,0x00],
                        'g': [0x18,0xA4,0xA4,0x7C,0x00],
                        'h': [0x7E,0x08,0x08,0x70,0x00],
                        'i': [0x00,0x48,0x7A,0x40,0x00],
                        'j': [0x40,0x80,0x88,0x7A,0x00],
                        'k': [0x7E,0x10,0x28,0x44,0x00],
                        'l': [0x00,0x42,0x7E,0x40,0x00],
                        'm': [0x7C,0x04,0x18,0x04,0x78],
                        'n': [0x7C,0x04,0x04,0x78,0x00],
                        'o': [0x38,0x44,0x44,0x38,0x00],
                        'p': [0xFC,0x24,0x24,0x18,0x00],
                        'q': [0x18,0x24,0x24,0xFC,0x00],
                        'r': [0x7C,0x08,0x04,0x00,0x00],
                        's': [0x48,0x54,0x54,0x24,0x00],
                        't': [0x04,0x3E,0x44,0x00,0x00],
                        'u': [0x3C,0x40,0x40,0x7C,0x00],
                        'v': [0x1C,0x20,0x40,0x20,0x1C],
                        'w': [0x3C,0x40,0x30,0x40,0x3C],
                        'x': [0x44,0x28,0x10,0x28,0x44],
                        'y': [0x1C,0xA0,0xA0,0x7C,0x00],
                        'z': [0x44,0x64,0x54,0x4C,0x00]
                        };
                    const pattern = font5x7[char];

                    // Draw each column of the character
                    for (let col = 0; col < 5; col++) {
                        // Check if we're still within display bounds
                        if (x + col >= 32) break;

                        // Get column pattern
                        const colPattern = pattern[col];

                        // Draw each pixel in the column
                        for (let row = 0; row < 7; row++) {
                            if (y + row >= 8) break;

                            if (colPattern & (1 << row)) {
                                // Set pixel color based on color parameter:
                                // 1 = Red (red only)
                                // 2 = Green (green only)
                                // 3 = Yellow (both red and green)
                                const red = color & 1;
                                const green = (color & 2) >> 1;
                                setPixel(x + col, y + row, red, green);
                            }
                        }
                    }
                }

                async function updateDisplay() {
                    // This function should be called after any changes to refresh the display

                    // Send the current buffer state to the LED matrix
                    for (let row = 0; row < 8; row++) {
                        // Convert buffer rows to bit patterns
                        let redPattern = 0;
                        let greenPattern = 0;

                        for (let col = 0; col < 32; col++) {
                            // Shift patterns left and add new bits
                            redPattern = (redPattern << 1) | redBuffer[row][col];
                            greenPattern = (greenPattern << 1) | greenBuffer[row][col];
                        }
                    }

                    // Trigger display update
                    refreshDisplay();
                }

                function setPixel(x, y, red, green) {
                    // Set pixel color in the buffer
                    if (x < 0 || x >= 32 || y < 0 || y >= 8) return; // Out of bounds
                    redBuffer[y][x] = red ? 1 : 0;
                    greenBuffer[y][x] = green ? 1 : 0;
                }

                function refreshDisplay(blank = false) {
                    // This function updates the display based on the current buffer state
                    const pixels = document.querySelectorAll('.led-pixel');
                    for (let y = 0; y < 8; y++) {
                        for (let x = 0; x < 32; x++) {
                            const idx = y * 32 + x;
                            let color = 'rgb(31, 41, 55)'; // Default (off)
                            if (redBuffer[y][x] && greenBuffer[y][x]) {
                                color = 'yellow';
                            } else if (redBuffer[y][x]) {
                                color = 'red';
                            } else if (greenBuffer[y][x]) {
                                color = 'lime';
                            }
                            pixels[idx].style.backgroundColor = blank ? 'rgb(31, 41, 55)' : color;
                        }
                    }
                }

                async function displayColoredText(charArray) {
                    // Clear any previous patterns
                    await clearDisplay();

                    let xPos = 1;
                    for (let i = 0; i < charArray.length && xPos < 32; i++) {
                        const { char, color, size } = charArray[i];
                        console.log(char)
                        drawChar(xPos, 0, char, color);
                        xPos += size ? size : 5; // 5 pixels for char + 1 space
                    }
                    updateDisplay();
                }

                // Load current settings when page loads
                $.get('/get-settings', function(data) {
                    $('#hourColor').val(data.hour_color);
                    $('#minuteColor').val(data.minute_color);
                    $('#secondsColor').val(data.seconds_color);
                    $('#colonMode').val(data.colon_mode);
                    $('#colonColor').val(data.colon_color);
                });

                // Event listeners
                $('#refreshRate').on('change', setRefreshRate);
                $('#brightnessControl').on('input', updateMatrixDisplay);
                $('#saveSettings').on('click', updateClockSettings);

                $('#liveToggle').on('click', function() {
                    isLive = !isLive;
                    $(this).toggleClass('live-active');
                    if (previewLoop) {
                        console.log('Stopping preview loop');
                        previewLoop = false;
                        if (previewInterval) {
                            clearInterval(previewInterval);
                            previewInterval = null;
                            clearDisplay();
                        }
                        $('#previewSwitch').prop('checked', false);
                        $('#shortTimeSwitch').prop('checked', false);
                    }

                    if (isLive) {
                        liveInterval = setInterval(updateMatrixDisplay, parseInt($('#refreshRate').val()));
                    } else {
                        clearInterval(liveInterval);
                        clearDisplay();
                    }
                });

                $('#clearBtn').on('click', function() {
                    clearDisplay();
                });
                let blinkingColon = false;
                let shortHourMode = false;

                $('#previewSwitch').on('change', function() {
                    if (this.checked) {
                        previewLoop = true;
                        if (previewInterval) clearInterval(previewInterval);
                        previewInterval = setInterval(async function() {
                            if (!previewLoop) return;
                            const settings = {
                                hour_color: parseInt($('#hourColor').val()),
                                minute_color: parseInt($('#minuteColor').val()),
                                seconds_color: parseInt($('#secondsColor').val()),
                                colon_color: parseInt($('#colonColor').val()),
                                colon_mode: parseInt($('#colonMode').val())
                            };
                            console.log('Previewing with settings:', settings, ' previewLoop:', previewLoop);
                            if (settings.colon_mode === 0) {
                                displayColoredText([
                                    { char: '1', color: settings.hour_color },
                                    { char: '2', color: settings.hour_color },
                                    { char: '3', color: settings.minute_color },
                                    { char: '4', color: settings.minute_color },
                                    { char: shortHourMode ? 'p' : '5', color: settings.seconds_color },
                                    { char: shortHourMode ? 'm' : '6', color: settings.seconds_color }
                                ]);
                            } else if (settings.colon_mode === 1) {
                                displayColoredText([
                                    { char: '1', color: settings.hour_color },
                                    { char: '2', color: settings.hour_color, size: 4 },
                                    { char: ':', color: settings.colon_color, size: 1 },
                                    { char: '3', color: settings.minute_color },
                                    { char: '4', color: settings.minute_color, size: 4 },
                                    { char: shortHourMode ? ' ' : ':', color: settings.colon_color, size: 1 },
                                    { char: shortHourMode ? 'p' : '5', color: settings.seconds_color },
                                    { char: shortHourMode ? 'm' : '6', color: settings.seconds_color }
                                ]);
                            } else if (settings.colon_mode === 2) {
                                displayColoredText([
                                    { char: '1', color: settings.hour_color },
                                    { char: '2', color: settings.hour_color, size: 4 },
                                    { char: blinkingColon ? ':' : ' ', color: settings.colon_color, size: 1 },
                                    { char: '3', color: settings.minute_color },
                                    { char: '4', color: settings.minute_color, size: 4 },
                                    { char: shortHourMode ? ' ' : blinkingColon ? ':' : ' ', color: settings.colon_color, size: 1 },
                                    { char: shortHourMode ? 'p' : '5', color: settings.seconds_color },
                                    { char: shortHourMode ? 'm' : '6', color: settings.seconds_color }
                                ]);
                                blinkingColon = !blinkingColon;
                            } else if (settings.colon_mode === 3) {
                                displayColoredText([
                                    { char: '1', color: settings.hour_color },
                                    { char: '2', color: settings.hour_color, size: 4 },
                                    { char: '.', color: settings.colon_color, size: 1 },
                                    { char: '3', color: settings.minute_color },
                                    { char: '4', color: settings.minute_color, size: 4 },
                                    { char: shortHourMode ? ' ' : '.', color: settings.colon_color, size: 1 },
                                    { char: shortHourMode ? 'p' : '5', color: settings.seconds_color },
                                    { char: shortHourMode ? 'm' : '6', color: settings.seconds_color }
                                ]);
                            } else if (settings.colon_mode === 4) {
                                displayColoredText([
                                    { char: '1', color: settings.hour_color },
                                    { char: '2', color: settings.hour_color, size: 4 },
                                    { char: blinkingColon ? '.' : ' ', color: settings.colon_color, size: 1 },
                                    { char: '3', color: settings.minute_color },
                                    { char: '4', color: settings.minute_color, size: 4 },
                                    { char: shortHourMode ? ' ' : blinkingColon ? '.' : ' ', color: settings.colon_color, size: 1 },
                                    { char: shortHourMode ? 'p' : '5', color: settings.seconds_color },
                                    { char: shortHourMode ? 'm' : '6', color: settings.seconds_color }
                                ]);
                                blinkingColon = !blinkingColon;
                            }
                        }, 1000);

                        if (isLive) {
                            console.log('Stopping live preview');
                            isLive = false;
                            $('#liveToggle').removeClass('live-active');
                            clearInterval(liveInterval);
                            clearDisplay();
                        }
                    } else {
                        previewLoop = false;
                        if (previewInterval) {
                            clearInterval(previewInterval);
                            previewInterval = null;
                        }
                        clearDisplay();
                    }
                });

                $('#shortTimeSwitch').on('change', function() {
                    const icon = $('#shortTimeIcon');
                    if (this.checked) {
                        shortHourMode = true;
                        icon.removeClass('mdi-sun text-yellow-500').addClass('mdi-weather-night text-blue-800');
                    } else {
                        shortHourMode = false;
                        icon.removeClass('mdi-weather-night text-blue-800').addClass('mdi-sun text-yellow-500');
                    }
                });
                // Cleanup on page change
                window.addEventListener('hashchange', () => {
                    if (refreshInterval) clearInterval(refreshInterval);
                    previewLoop = false;
                    if (previewInterval) {
                        clearInterval(previewInterval);
                        previewInterval = null;
                    }
                    clearDisplay();
                });
            }

        }

        async function sleep(ms) {
            return new Promise((resolve) => {
                setTimeout(resolve, ms);
            });
        }

        // --- Status Box Polling with WiFi Icon ---
        function updateStatusBox() {
            $.get('/status', function(data) {
                // Example response: { internet: true, ip: '192.168.1.123' }
                if (data.internet) {
                    $('#wifiIcon').removeClass('mdi-wifi-off text-red-500 text-gray-400').addClass('mdi-wifi text-green-500');
                } else {
                    $('#wifiIcon').removeClass('mdi-wifi text-green-500 text-gray-400').addClass('mdi-wifi-off text-red-500');
                }
                $('#ipAddress').text('IP: ' + (data.ip || '--'));
            }).fail(function() {
                $('#wifiIcon').removeClass('mdi-wifi text-green-500 mdi-wifi-off text-red-500').addClass('mdi-wifi text-gray-400');
                $('#ipAddress').text('IP: --');
            });
        }

        setInterval(updateStatusBox, 3000);
        updateStatusBox();

        $(function() {
            setDarkMode(localStorage.getItem('darkMode') === '1');
        });

        $('#darkModeSwitch').on('change', function() {
            setDarkMode(this.checked);
        });

        $('#statusBox').on('click', function() {
            // copy to clipboard
            const ip = $('#ipAddress').text().replace('IP: ', '');
            if (ip === '--') {
                alert('No IP address available to copy.');
                return;
            }
            navigator.clipboard.writeText(ip).then(() => {
                alert('IP address copied to clipboard: ' + ip);
            }).catch(err => {
                console.error('Failed to copy IP address:', err);
            });
        });

        $(window).on('hashchange', function() {
            loadPage(location.hash);
        });
        // Initial load
        loadPage(location.hash);
    </script>
    </body>
    </html>
    )rawliteral";