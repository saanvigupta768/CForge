$cases = @(
    @{ Name = "no loops";          File = "tests/cases/no_loops.c";         Expect = "O(1) loop-related work";    ShouldFail = $false },
    @{ Name = "sequential loops";  File = "tests/cases/sequential_loops.c"; Expect = "approximately O(n)";        ShouldFail = $false },
    @{ Name = "nested loops";      File = "examples/nested_loops.c";        Expect = "approximately O(n^2)";      ShouldFail = $false },
    @{ Name = "triple nested";     File = "tests/cases/triple_nested.c";    Expect = "approximately O(n^3)";      ShouldFail = $false },
    @{ Name = "constant loop";     File = "tests/cases/const_loop.c";       Expect = "approximately O(1) (all loops have constant bounds)"; ShouldFail = $false },
    @{ Name = "logarithmic loop";  File = "tests/cases/log_loop.c";         Expect = "approximately O(log n)";    ShouldFail = $false },
    @{ Name = "n log n";           File = "tests/cases/n_log_n.c";          Expect = "approximately O(n log n)";  ShouldFail = $false },
    @{ Name = "infinite loop";     File = "tests/cases/bad_loop.c";         Expect = "Unknown";                   ShouldFail = $false },
    @{ Name = "if / else";         File = "tests/cases/if_else.c";          Expect = "If";                        ShouldFail = $false },
    @{ Name = "while loop";        File = "tests/cases/while_loop.c";       Expect = "while loops are not analyzed yet"; ShouldFail = $false },
    @{ Name = "array duplicates";  File = "tests/cases/array_dupes.c";      Expect = "approximately O(n^2)";      ShouldFail = $false },
    @{ Name = "missing semicolon"; File = "tests/cases/bad_syntax.c";       Expect = "Expected ';' at line 3";    ShouldFail = $true },
    @{ Name = "missing paren";     File = "tests/cases/bad_if.c";           Expect = "Expected ')' at line 4";    ShouldFail = $true }
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