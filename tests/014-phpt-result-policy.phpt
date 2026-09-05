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

function runChecker(string $checker, string $results, string $tests, string ...$options): array
{
    $process = proc_open(
        [PHP_BINARY, '-n', $checker, $results, $tests, ...$options],
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
printResult('extra-option', runChecker(
    $checker,
    $results,
    $tests,
    '--require-qualification',
    '--unexpected',
));
printResult('qualification-skipped', runChecker($checker, $results, $tests, '--require-qualification'));

file_put_contents($results, "PASSED\t$pass\nPASSED\t$qualification\n");
printResult('qualification-passed', runChecker($checker, $results, $tests, '--require-qualification'));
printResult('ordinary-rejects-qualification-pass', runChecker($checker, $results, $tests));

file_put_contents($results, "SKIPPED\t$pass\nPASSED\t$qualification\n");
printResult('qualification-rejects-other-skip', runChecker($checker, $results, $tests, '--require-qualification'));

file_put_contents($results, "PASSED\t$pass\n");
printResult('qualification-result-missing', runChecker($checker, $results, $tests, '--require-qualification'));

unlink($qualification);
file_put_contents($results, "PASSED\t$pass\n");
printResult('valid-without-intentional-skip', runChecker($checker, $results, $tests));
printResult('qualification-test-missing', runChecker($checker, $results, $tests, '--require-qualification'));
file_put_contents($qualification, '');

file_put_contents($results, "SKIPPED\t$pass\nSKIPPED\t$qualification\n");
printResult('unexpected-skip', runChecker($checker, $results, $tests));
printResult('missing-module-qualification', runChecker($checker, $results, $tests, '--require-qualification'));

printResult('unknown-option', runChecker($checker, $results, $tests, '--require-qualificaton'));

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

extra-option
exit=1
stdout:

stderr:
Usage: check-phpt-results.php RESULTS TESTS [--require-qualification]
qualification-skipped
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 012-build-qualification.phpt: expected PASSED, got SKIPPED
qualification-passed
exit=0
stdout:
PHPT status policy passed: 2 tests, 0 intentional skips.
stderr:

ordinary-rejects-qualification-pass
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 012-build-qualification.phpt: expected SKIPPED, got PASSED
qualification-rejects-other-skip
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 001-pass.phpt: expected PASSED, got SKIPPED
qualification-result-missing
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 012-build-qualification.phpt: missing result
valid-without-intentional-skip
exit=0
stdout:
PHPT status policy passed: 1 tests, 0 intentional skips.
stderr:

qualification-test-missing
exit=1
stdout:

stderr:
Cannot find build qualification test.
unexpected-skip
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 001-pass.phpt: expected PASSED, got SKIPPED
missing-module-qualification
exit=1
stdout:

stderr:
Unexpected PHPT results:
- 001-pass.phpt: expected PASSED, got SKIPPED
- 012-build-qualification.phpt: expected PASSED, got SKIPPED
unknown-option
exit=1
stdout:

stderr:
Usage: check-phpt-results.php RESULTS TESTS [--require-qualification]
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
