
function Get-SHA256Hash([byte[]]$data) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $hash = $sha.ComputeHash($data)
    $sha.Dispose()
    return $hash
}

function Decrypt-File([string]$encFilePath, [byte[]]$keyBytes) {
    Write-Host "  [Decrypting] $encFilePath" -ForegroundColor Cyan

    # 读取加密文件
    try {
        $encBytes = [System.IO.File]::ReadAllBytes($encFilePath)
    } catch {
        Write-Host "  [ERROR] Cannot read: $encFilePath" -ForegroundColor Red
        return $false
    }

    # 检查文件长度是否至少为 32+16=48 字节
    if ($encBytes.Length -lt 48) {
        Write-Host "  [ERROR] File too small, invalid: $encFilePath" -ForegroundColor Red
        return $false
    }

    # 提取头部哈希（前32字节）
    $storedHash = New-Object byte[] 32
    [System.Array]::Copy($encBytes, 0, $storedHash, 0, 32)

    # 提取 IV（接下来16字节）
    $iv = New-Object byte[] 16
    [System.Array]::Copy($encBytes, 32, $iv, 0, 16)

    # 提取密文（剩余部分）
    $cipherBytes = New-Object byte[] ($encBytes.Length - 48)
    [System.Array]::Copy($encBytes, 48, $cipherBytes, 0, $cipherBytes.Length)

    # 计算当前密码的哈希，与存储的哈希比较
    $computedHash = Get-SHA256Hash $keyBytes
    $hashMatch = $true
    for ($i = 0; $i -lt 32; $i++) {
        if ($storedHash[$i] -ne $computedHash[$i]) {
            $hashMatch = $false
            break
        }
    }

    if (-not $hashMatch) {
        Write-Host "  [ERROR] Password hash mismatch, wrong password? $encFilePath" -ForegroundColor Red
        return $false
    }

    # AES 解密器
    $aes = [System.Security.Cryptography.Aes]::Create()
    $aes.KeySize = 256
    $aes.Mode = [System.Security.Cryptography.CipherMode]::CBC
    $aes.Padding = [System.Security.Cryptography.PaddingMode]::PKCS7
    $aes.Key = $keyBytes
    $aes.IV = $iv

    $decryptor = $aes.CreateDecryptor()
    try {
        $plainBytes = $decryptor.TransformFinalBlock($cipherBytes, 0, $cipherBytes.Length)
    } catch {
        Write-Host "  [ERROR] Decryption failed: $encFilePath" -ForegroundColor Red
        return $false
    } finally {
        $aes.Dispose()
    }

    # 构造原文件名（去掉 .xdhjm）
    $origFile = $encFilePath -replace '\.xdhjm$', ''
    if ($origFile -eq $encFilePath) {
        Write-Host "  [ERROR] Filename does not end with .xdhjm: $encFilePath" -ForegroundColor Red
        return $false
    }

    # 写入原文件
    try {
        [System.IO.File]::WriteAllBytes($origFile, $plainBytes)
    } catch {
        Write-Host "  [ERROR] Failed to write original file: $origFile" -ForegroundColor Red
        return $false
    }

    # 删除加密文件
    try {
        [System.IO.File]::Delete($encFilePath)
    } catch {
        Write-Host "  [WARN] Cannot delete encrypted file: $encFilePath" -ForegroundColor Yellow
    }

    Write-Host "  [OK] $encFilePath -> $origFile" -ForegroundColor Green
    return $true
}

function Decrypt-Directory([string]$dirPath, [byte[]]$keyBytes) {
    if (-not (Test-Path $dirPath -PathType Container)) {
        Write-Host "[WARN] Directory not found: $dirPath" -ForegroundColor Yellow
        return
    }

    Write-Host "[Processing] $dirPath" -ForegroundColor Magenta

    $items = Get-ChildItem -Path $dirPath -Force -ErrorAction SilentlyContinue
    if (-not $items) { return }

    foreach ($item in $items) {
        if ($item.PSIsContainer) {
            Decrypt-Directory $item.FullName $keyBytes
        } else {
            if ($item.Extension -ieq '.xdhjm') {
                Decrypt-File $item.FullName $keyBytes
            }
        }
    }
}

# ========== 主程序 ==========
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host '  Full Disk Decryptor (AES-256-CBC)' -ForegroundColor Yellow
Write-Host '  Password rule: 2011 - (value in C:\syslock_app\passw.txt)' -ForegroundColor Yellow
Write-Host '  This will decrypt all .xdhjm files and restore originals.' -ForegroundColor Yellow
Write-Host '  Make sure the password file has the correct value used for encryption.' -ForegroundColor Yellow
Write-Host '============================================================' -ForegroundColor Cyan

# 读取密码文件
$pwdFile = 'C:\syslock_app\passw.txt'
if (-not (Test-Path $pwdFile)) {
    Write-Host "ERROR: Password file not found: $pwdFile" -ForegroundColor Red
    exit 1
}

$pwdText = (Get-Content $pwdFile -Raw).Trim()
$num = 0
if (-not ([int]::TryParse($pwdText, [ref]$num))) {
    Write-Host 'ERROR: Password file content is not a valid integer.' -ForegroundColor Red
    exit 1
}

$passwordInt = 2011 - $num
if ($passwordInt -lt 0) { $passwordInt = 0 }
$passwordStr = $passwordInt.ToString()
Write-Host "Password: $passwordStr" -ForegroundColor Green

# 生成 AES-256 密钥
$keyBytes = Get-SHA256Hash ([System.Text.Encoding]::UTF8.GetBytes($passwordStr))

# 获取所有固定盘和可移动盘
$drives = [System.IO.DriveInfo]::GetDrives() | Where-Object { $_.DriveType -eq 'Fixed' -or $_.DriveType -eq 'Removable' }

foreach ($drive in $drives) {
    $root = $drive.RootDirectory.FullName
    Write-Host "`n[Decrypting on drive $root]" -ForegroundColor Magenta
    Decrypt-Directory $root $keyBytes
}

Write-Host "`nDecryption task completed." -ForegroundColor Cyan