--TEST--
yumemi CI rejects unexpected PHPT result states
--FILE--
<?php
$checker = dirname(__DIR__) . '/scripts/check-phpt-results.php';
$fixture = sys_get_temp_dir() . '/yumemi-phpt-results-' . getmypid();
$tests = $fixture . '/tests';
$results = $fixture . '/results.txt';

mkdir($fixture);
mkdir($tests);
file_put_contents($tests . '/001-pass.phpt', '');
file_put_contents($tests . '/012-build-qualification.phpt', '');

function runChecker(string $checker, string $results, string $tests): array
{
    $process = proc_open(
        [PHP_BINARY, '-n', $checker, $results, $tests],
        [
            1 => ['pipe', 'w'],
            2 => ['pipe', 'w'],
        ],
        $pipes,
    );

    $stdout = stream_get_contents($pipes[1]);
    fclose($pipes[1]);
    $stderr = stream_get_contents($pipes[2]);
    fclose($pipes[2]);

    return [proc_close($process), trim($stdout), trim($stderr)];
}

function printResult(string $label, array $result): void
{
    [$exitCode, $stdout, $stderr] = $result;
    echo $label, "\n";
    echo 'exit=', $exitCode, "\n";
    echo "stdout:\n", $stdout, "\n";
    echo "stderr:\n", $stderr, "\n";
}

$pass = $tests . '/001-pass.phpt';
$qualification = $tests . '/012-build-qualification.phpt';

file_put_contents($results, "PASSED\t$pass\nSKIPPED\t$qualification\n");
printResult('valid', runChecker($checker, $results, $tests));

unlink($qualification);
file_put_contents($results, "PASSED\t$pass\n");
printResult('valid-without-intentional-skip', runChecker($checker, $results, $tests));
file_put_contents($qualification, '');

file_put_contents($results, "SKIPPED\t$pass\nSKIPPED\t$qualification\n");
printResult('unexpected-skip', runChecker($checker, $results, $tests));

file_put_contents($results, "not-a-record\n");
printResult('malformed', runChecker($checker, $results, $tests));

printResult('missing', runChecker($checker, $fixture . '/missing.txt', $tests));

unlink($results);
unlink($tests . '/001-pass.phpt');
unlink($tests . '/012-build-qualification.phpt');
rmdir($tests);
rmdir($fixture);
?>
--EXPECT--
valid
exit=0
stdout:
PHPT status policy passed: 2 tests, 1 intentional skip.
stderr:

valid-without-intentional-skip
exit=0
stdout:
PHPT status policy passed: 1 tests, 0 intentional skips.
stderr:

unexpected-skip
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 001-pass.phpt: expected PASSED, got SKIPPED
malformed
exit=1
stdout:

stderr:
Invalid PHPT result line 1: expected STATUS<TAB>PATH.
missing
exit=1
stdout:

stderr:
Cannot read PHPT result file.
