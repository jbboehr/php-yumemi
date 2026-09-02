#!/usr/bin/env php
<?php

/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
 *
 * SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
 */

declare(strict_types=1);

if ($argc !== 3) {
    fwrite(STDERR, "Usage: check-phpt-results.php RESULTS TESTS\n");
    exit(1);
}

$resultLines = @file($argv[1], FILE_IGNORE_NEW_LINES);
if ($resultLines === false) {
    fwrite(STDERR, "Cannot read PHPT result file.\n");
    exit(1);
}

$testFiles = glob(rtrim($argv[2], '/\\') . DIRECTORY_SEPARATOR . '*.phpt');
if ($testFiles === false || $testFiles === []) {
    fwrite(STDERR, "Cannot find PHPT tests.\n");
    exit(1);
}

$expected = [];
foreach ($testFiles as $testFile) {
    $name = basename($testFile);
    $expected[$name] = $name === '012-build-qualification.phpt' ? 'SKIPPED' : 'PASSED';
}

$actual = [];
foreach ($resultLines as $index => $line) {
    $fields = explode("\t", $line, 2);
    if (count($fields) !== 2 || $fields[0] === '' || $fields[1] === '') {
        fwrite(STDERR, sprintf(
            "Invalid PHPT result line %d: expected STATUS<TAB>PATH.\n",
            $index + 1,
        ));
        exit(1);
    }

    $name = basename(str_replace('\\', '/', $fields[1]));
    if (isset($actual[$name])) {
        fwrite(STDERR, "Duplicate PHPT result for {$name}.\n");
        exit(1);
    }
    $actual[$name] = $fields[0];
}

$errors = [];
foreach ($expected as $name => $status) {
    if (!isset($actual[$name])) {
        $errors[] = "{$name}: missing result";
    } elseif ($actual[$name] !== $status) {
        $errors[] = "{$name}: expected {$status}, got {$actual[$name]}";
    }
}
foreach ($actual as $name => $status) {
    if (!isset($expected[$name])) {
        $errors[] = "{$name}: unexpected {$status} result";
    }
}

if ($errors !== []) {
    fwrite(STDERR, "Unexpected PHPT results:\n- " . implode("\n- ", $errors) . "\n");
    exit(1);
}

printf(
    "PHPT status policy passed: %d tests, 1 intentional skip.\n",
    count($expected),
);
