$cases = @(
    @{ Name = "no loops";          File = "tests/cases/no_loops.c";         Expect = "O(1) loop-related work"; ShouldFail = $false },
    @{ Name = "sequential loops";  File = "tests/cases/sequential_loops.c"; Expect = "approximately O(n)";     ShouldFail = $false },
    @{ Name = "nested loops";      File = "examples/nested_loops.c";        Expect = "approximately O(n^2)";   ShouldFail = $false },
    @{ Name = "triple nested";     File = "tests/cases/triple_nested.c";    Expect = "approximately O(n^3)";   ShouldFail = $false },
    @{ Name = "missing semicolon"; File = "tests/cases/bad_syntax.c";       Expect = "Expected ';' at line 3"; ShouldFail = $true }
)

$passed = 0

foreach ($c in $cases) {
    $output = (& .\cforge.exe $c.File 2>&1 | Out-String)
    $exitCode = $LASTEXITCODE

    $ok = $output.Contains($c.Expect)

    if ($c.ShouldFail) {
        $ok = $ok -and ($exitCode -ne 0)
    } else {
        $ok = $ok -and ($exitCode -eq 0)
    }

    if ($ok) {
        Write-Host "PASS  $($c.Name)" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "FAIL  $($c.Name)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "$passed of $($cases.Count) tests passed"

if ($passed -ne $cases.Count) { exit 1 }