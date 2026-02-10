# Test Cluster Script for Narwhal C++ Port (Windows)
param(
    [string]$Engine = "tusk",
    [switch]$Load
)

# Configuration
$Nodes = 4
$BasePrimaryPort = 8000
$BaseWorkerPort = 9000

# Resolve absolute paths
$ExecutablePrimary = Join-Path $PSScriptRoot "build/Release/primary_node.exe"
$ExecutableWorker = Join-Path $PSScriptRoot "build/Release/worker_node.exe"

# 0. Check if executables exist
if (!(Test-Path $ExecutablePrimary) -or !(Test-Path $ExecutableWorker)) {
    Write-Error "Executables not found at $ExecutablePrimary. Did you compile the project in Release mode?"
    exit
}

# 1. Generate Dummy Certificates/Keys if they don't exist
if (!(Test-Path "cert.pem")) {
    Write-Host "Creating dummy certificate files..."
    "DUMMY CERT" | Out-File -FilePath "cert.pem"
    "DUMMY KEY" | Out-File -FilePath "key.pem"
    "DUMMY CERT" | Out-File -FilePath "worker_cert.pem"
    "DUMMY KEY" | Out-File -FilePath "worker_key.pem"
}

Write-Host "Starting Narwhal Cluster ($Nodes nodes) with engine: $Engine..." -ForegroundColor Cyan

# 2. Launch Primaries and Workers
$Jobs = @()

for ($i = 0; $i -lt $Nodes; $i++) {
    $PrimaryPort = $BasePrimaryPort + $i
    $WorkerPort = $BaseWorkerPort + $i
    $DbPathPrimary = "./db_primary_$i"
    $DbPathWorker = "./db_worker_$i"

    Write-Host "Launching Node $i (Primary: $PrimaryPort, Worker: $WorkerPort)..."
    
    $ArgsPrimary = "--port $PrimaryPort --db $DbPathPrimary --engine $Engine"
    if ($Load) { $ArgsPrimary += " --load" }
    
    # Launch in new windows to see output clearly, keeping windows open with -NoExit
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd '$PSScriptRoot'; & '$ExecutablePrimary' $ArgsPrimary"
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd '$PSScriptRoot'; & '$ExecutableWorker' --port $WorkerPort --db $DbPathWorker"
}

Write-Host "Cluster is running. Press Ctrl+C to stop." -ForegroundColor Yellow

try {
    while ($true) { Start-Sleep -Seconds 1 }
}
finally {
    Write-Host "Stopping cluster..." -ForegroundColor Red
    foreach ($j in $Jobs) {
        Stop-Process -Id $j.Id -ErrorAction SilentlyContinue -Force
    }
}
