# Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
#
# SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
{
  lib,
  php,
  src,
  checkSupport ? false,
}:
let
  romicException = {
    licenseType = "simple";
    shortName = "romic-exception";
    fullName = "Romic Exception";
    spdxId = "romic-exception";
    url = "https://spdx.org/licenses/romic-exception.html";
    free = true;
    redistributable = true;
    deprecated = false;
  };
  ucarLicense = {
    licenseType = "permissive";
    shortName = "UCAR";
    fullName = "University Corporation for Atmospheric Research License";
    spdxId = "UCAR";
    free = true;
    redistributable = true;
    deprecated = false;
  };
in
(php.buildPecl {
  pname = "yumemi";
  version = "0.1.0-dev";

  inherit src;

  nativeBuildInputs = [ php.unwrapped.dev ];
  configureFlags = [ "--enable-yumemi" ];

  doCheck = checkSupport;

  passthru = { inherit php; };

  meta = {
    description = "Experimental native PHP extension companion to yumemi.php";
    homepage = "https://github.com/jbboehr/php-yumemi";
    license = [
      (lib.licenses.WITH lib.licenses.agpl3Only romicException)
      ucarLicense
    ];
    platforms = lib.platforms.unix;
  };
}).overrideAttrs
  {
    checkPhase = ''
      runHook preCheck
      REPORT_EXIT_STATUS=1 NO_INTERACTION=1 make test TEST_PHP_ARGS="-n" \
        || (find tests -name '*.log' -exec cat {} \; ; exit 1)
      runHook postCheck
    '';
  }
