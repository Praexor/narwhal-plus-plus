# Narwhal C++ Benchmarking Script
param(
    [int]$Duration = 15
)

$Engines = "tusk", "shoal++", "mysticeti"
$Results = @()

foreach ($Engine in $Engines) {
    Write-Host "`n--- Testing Engine: $Engine ---" -ForegroundColor Cyan
    
    # Start the cluster with load generator enabled
    $Process = Start-Process powershell -ArgumentList "-File ./run_cluster.ps1 -Engine $Engine -Load" -PassThru -NoNewWindow
    
    Write-Host "Waiting $Duration seconds for data collection..."
    Start-Sleep -Seconds $Duration
    
    # Stop the cluster
    Write-Host "Stopping cluster..."
    # Killing node processes directly to be sure
    Stop-Process -Name primary_node,worker_node -Force -ErrorAction SilentlyContinue
    
    Write-Host "Benchmark for $Engine completed."
}

Write-Host "`nBenchmarking Finished." -ForegroundColor Green
