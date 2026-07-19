#ifndef WEBAPP_H
#define WEBAPP_H

#include <Arduino.h>

const char* webapp_html PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Mini GIF Frame</title>
    <link href="https://fonts.googleapis.com/css2?family=Noto+Sans+KR:wght@400;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-color: #0f172a;
            --glass-bg: rgba(255, 255, 255, 0.1);
            --glass-border: rgba(255, 255, 255, 0.2);
            --primary: #3b82f6;
            --primary-hover: #2563eb;
            --text-main: #f8fafc;
            --text-muted: #cbd5e1;
        }

        body {
            font-family: 'Noto Sans KR', sans-serif;
            background: linear-gradient(135deg, var(--bg-color), #1e293b);
            color: var(--text-main);
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
        }

        .container {
            background: var(--glass-bg);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border: 1px solid var(--glass-border);
            border-radius: 24px;
            padding: 40px 30px;
            width: 90%;
            max-width: 400px;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            text-align: center;
            animation: fadeIn 0.8s ease-out;
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }

        h1 {
            font-size: 24px;
            margin-bottom: 10px;
            font-weight: 700;
            background: linear-gradient(to right, #60a5fa, #a78bfa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        p.subtitle {
            color: var(--text-muted);
            font-size: 14px;
            margin-bottom: 30px;
        }

        .upload-box {
            border: 2px dashed var(--glass-border);
            border-radius: 16px;
            padding: 40px 20px;
            cursor: pointer;
            transition: all 0.3s ease;
            position: relative;
            background: rgba(0, 0, 0, 0.2);
            margin-bottom: 24px;
        }

        .upload-box:hover {
            border-color: var(--primary);
            background: rgba(59, 130, 246, 0.1);
        }

        .upload-box input[type="file"] {
            position: absolute;
            top: 0; left: 0; width: 100%; height: 100%;
            opacity: 0;
            cursor: pointer;
        }

        .upload-icon {
            font-size: 48px;
            margin-bottom: 10px;
        }

        .preview-container {
            display: none;
            margin-bottom: 24px;
        }

        .preview-image {
            max-width: 128px;
            max-height: 128px;
            border-radius: 12px;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.3);
            border: 2px solid var(--glass-border);
        }

        .btn {
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 12px;
            padding: 14px 24px;
            font-size: 16px;
            font-weight: 700;
            cursor: pointer;
            width: 100%;
            transition: background 0.3s ease, transform 0.1s ease;
            box-shadow: 0 4px 6px -1px rgba(59, 130, 246, 0.3);
        }

        .btn:hover {
            background: var(--primary-hover);
            transform: translateY(-2px);
        }

        .btn:active {
            transform: translateY(0);
        }

        .btn:disabled {
            background: #475569;
            cursor: not-allowed;
            transform: none;
        }

        .progress-bar {
            width: 100%;
            height: 8px;
            background: var(--glass-border);
            border-radius: 4px;
            margin-top: 20px;
            overflow: hidden;
            display: none;
        }

        .progress-fill {
            height: 100%;
            background: linear-gradient(to right, #3b82f6, #8b5cf6);
            width: 0%;
            transition: width 0.3s ease;
        }

        #status {
            margin-top: 15px;
            font-size: 14px;
            color: var(--text-muted);
        }
    </style>
</head>
<body>

<div class="container">
    <h1>Mini GIF Frame</h1>
    <p class="subtitle">사진이나 움짤(GIF)을 선택하여 전송하세요</p>

    <div class="upload-box" id="drop-area">
        <div class="upload-icon">📸</div>
        <div style="font-weight: 700; margin-bottom: 8px;">클릭하여 파일 선택</div>
        <div style="font-size: 12px; color: var(--text-muted);">지원 형식: JPG, JPEG, GIF<br>(최대 1.5MB)</div>
        <input type="file" id="file-input" accept=".jpg,.jpeg,.gif">
    </div>

    <div class="preview-container" id="preview-container">
        <img id="preview" class="preview-image" src="" alt="미리보기">
    </div>

    <button class="btn" id="upload-btn" disabled>액자로 전송하기 🚀</button>

    <div class="progress-bar" id="progress-bar">
        <div class="progress-fill" id="progress-fill"></div>
    </div>
    <div id="status"></div>
</div>

<script>
    const fileInput = document.getElementById('file-input');
    const previewContainer = document.getElementById('preview-container');
    const previewImage = document.getElementById('preview');
    const uploadBtn = document.getElementById('upload-btn');
    const dropArea = document.getElementById('drop-area');
    const progressBar = document.getElementById('progress-bar');
    const progressFill = document.getElementById('progress-fill');
    const statusText = document.getElementById('status');

    let selectedFile = null;

    fileInput.addEventListener('change', (e) => {
        if (e.target.files.length > 0) {
            selectedFile = e.target.files[0];
            
            // 용량 체크 (1.5MB 제한)
            if (selectedFile.size > 1.5 * 1024 * 1024) {
                alert('파일 용량이 너무 큽니다. 1.5MB 이하의 파일을 선택해주세요.');
                selectedFile = null;
                fileInput.value = '';
                previewContainer.style.display = 'none';
                uploadBtn.disabled = true;
                return;
            }

            const reader = new FileReader();
            reader.onload = (event) => {
                previewImage.src = event.target.result;
                previewContainer.style.display = 'block';
                uploadBtn.disabled = false;
                dropArea.style.display = 'none';
            };
            reader.readAsDataURL(selectedFile);
        }
    });

    uploadBtn.addEventListener('click', () => {
        if (!selectedFile) return;

        const formData = new FormData();
        formData.append('image', selectedFile);

        uploadBtn.disabled = true;
        uploadBtn.textContent = '전송 중...';
        progressBar.style.display = 'block';
        statusText.textContent = '업로드 중입니다...';

        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/upload', true);

        xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
                const percentComplete = (e.loaded / e.total) * 100;
                progressFill.style.width = percentComplete + '%';
            }
        };

        xhr.onload = () => {
            if (xhr.status === 200) {
                progressFill.style.background = '#10b981'; // Success green
                statusText.textContent = '전송 완료! 액자를 확인하세요 🎉';
                uploadBtn.textContent = '완료';
                setTimeout(() => {
                    // 리셋
                    progressFill.style.width = '0%';
                    progressFill.style.background = 'linear-gradient(to right, #3b82f6, #8b5cf6)';
                    progressBar.style.display = 'none';
                    dropArea.style.display = 'block';
                    previewContainer.style.display = 'none';
                    uploadBtn.disabled = true;
                    uploadBtn.textContent = '액자로 전송하기 🚀';
                    statusText.textContent = '';
                    fileInput.value = '';
                    selectedFile = null;
                }, 3000);
            } else {
                progressFill.style.background = '#ef4444'; // Error red
                statusText.textContent = '업로드 실패 😢 다시 시도해주세요.';
                uploadBtn.disabled = false;
                uploadBtn.textContent = '다시 전송하기';
            }
        };

        xhr.onerror = () => {
            progressFill.style.background = '#ef4444'; // Error red
            statusText.textContent = '네트워크 오류 발생 😢';
            uploadBtn.disabled = false;
            uploadBtn.textContent = '다시 전송하기';
        };

        xhr.send(formData);
    });
</script>

</body>
</html>
)=====";

#endif
