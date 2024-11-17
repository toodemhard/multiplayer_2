cmake --build .

if($LASTEXITCODE -eq 0) {
    Write-Host "success"

    ./idk.exe
} else {
    Write-Host "failure"
}
