cmake --build . --target $args[0]

if($LASTEXITCODE -eq 0) {
    Write-Host "success"

    Invoke-Expression "./$($args[0]).exe"
} else {
    Write-Host "failure"
}
