# Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
#
# SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
{
  nixpkgs,
  system,
  src,
  version,
  meta,
}:
let
  lib = nixpkgs.lib;
  pkgs = import nixpkgs {
    inherit system;
    config = {
      microsoftVisualStudioLicenseAccepted = true;
      allowUnfreePredicate =
        package:
        builtins.elem (lib.getName package) [
          "win-sdk"
          "xwin-fetch-msvc"
        ];
    };
  };
  # nixpkgs fetches its pinned MSVC CRT and Windows SDK through xwin.
  sdk = pkgs.pkgsCross.mingwW64.windows.sdk;
  llvm = pkgs.llvmPackages_21;

  # Windows development packs are versioned independently of nixpkgs' Unix PHP.
  # Hashes: https://downloads.php.net/~windows/releases/releases.json
  phpVersions = {
    php82 = {
      version = "8.2.33";
      compiler = "vs16";
      nts = "d4d0da6e6f1ad3f9e058262261fc43d7fa329b212207e4bfb2a39ad0b39ee891";
      ts = "46c00fc49b8cf6c35a4a8b5269114b1da90ef3a001f5ee1cd577f7c12ab6aad9";
    };
    php83 = {
      version = "8.3.33";
      compiler = "vs16";
      nts = "49fa1880cea4233b8c6128c84f01a1d5fc4de7e9c97854b1b832eef37f558a1c";
      ts = "d58cedfa76b74b49f88ee54d25426700a27dad76dbad4732440cf28ca8267a7c";
    };
    php84 = {
      version = "8.4.25";
      compiler = "vs17";
      nts = "55fd07f549c0494cfde2827dfe90ce128ff0359df7dbc76f7617ed1adcdefa48";
      ts = "75e4555ef32cb1524968f3019eb4567c06e89be13b1641148375fee99658b8b4";
    };
    php85 = {
      version = "8.5.10";
      compiler = "vs17";
      nts = "b277dafab9654b23dec28fdebe47385248fdd33bbe8ecdc33b8681ef1b5c7788";
      ts = "0031d279f13f21e81fd62f9a98e919f28b1875ba457916d60daed85586e479dd";
    };
  };

  makePackage =
    php: ts:
    let
      filename = "php-devel-pack-${php.version}${
        lib.optionalString (ts == "nts") "-nts"
      }-Win32-${php.compiler}-x64.zip";
      develPack = pkgs.fetchurl {
        urls = [
          "https://downloads.php.net/~windows/releases/${filename}"
          "https://downloads.php.net/~windows/releases/archives/${filename}"
        ];
        sha256 = php.${ts};
      };
      phpLibrary = if ts == "ts" then "php8ts" else "php8";
    in
    pkgs.stdenvNoCC.mkDerivation {
      pname = "php-yumemi-${lib.versions.majorMinor php.version}-windows-${ts}";
      inherit src version;

      strictDeps = true;
      nativeBuildInputs = [
        llvm.clang-unwrapped
        llvm.lld
        llvm.llvm
        pkgs.unzip
      ];
      dontConfigure = true;
      dontFixup = true;

      buildPhase = ''
        runHook preBuild
        unzip -q ${develPack} -d php-devel
        php="$PWD/php-devel/php-${php.version}-devel-${php.compiler}-x64"

        # Match the SDK's case on Unix filesystems.
        substituteInPlace "$php/include/main/streams/php_stream_transport.h" \
          --replace-fail '<Ws2tcpip.h>' '<WS2tcpip.h>'
        # MSVC-built PHP does not export Clang's specialized allocator functions.
        substituteInPlace "$php/include/Zend/zend_alloc.h" \
          --replace-fail '#if !ZEND_DEBUG && defined(HAVE_BUILTIN_CONSTANT_P)' '#if 0'
        ${lib.optionalString (lib.versionOlder php.version "8.4") ''
          # Older PHP headers exclude Clang from MSVC's vectorcall ABI.
          substituteInPlace "$php/include/Zend/zend_portability.h" \
            --replace-fail '#elif defined(_MSC_VER) && _MSC_VER >= 1800 && !defined(__clang__)' \
                           '#elif defined(_MSC_VER) && _MSC_VER >= 1800'
        ''}

        # PHP 8.3 uses _AddressOfReturnAddress without including intrin.h.
        clang-cl --target=x86_64-pc-windows-msvc -fuse-ld=lld \
          /vctoolsdir ${sdk}/crt /winsdkdir ${sdk}/sdk \
          /nologo /MD /O2 /LD /FIintrin.h -Wno-deprecated-declarations \
          /D_WINDOWS /DWINDOWS=1 /DWIN32 /DPHP_WIN32=1 /DZEND_WIN32=1 \
          /D_MBCS /D_USE_MATH_DEFINES /DENABLE_INTSAFE_SIGNED_FUNCTIONS \
          /DZEND_DEBUG=0 ${lib.optionalString (ts == "ts") "/DZTS=1"} \
          /DCOMPILE_DL_YUMEMI /DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 /DYY_NO_UNISTD_H=1 \
          /I. /I"$php/include" /I"$php/include/main" /I"$php/include/Zend" /I"$php/include/TSRM" \
          src/extension.c src/internal_quantity.c \
          src/parser/native_lexer.c src/parser/lexer_context.c \
          src/parser/native_parser.c src/parser/parser_context.c \
          src/parser/parser.c src/parser/scanner.c \
          /Fephp_yumemi.dll /link /libpath:"$php/lib" ${phpLibrary}.lib /Brepro
        runHook postBuild
      '';

      doCheck = true;
      checkPhase = ''
        runHook preCheck
        llvm-readobj --file-headers --coff-exports --coff-imports php_yumemi.dll > dll-info.txt
        grep -Fq 'Machine: IMAGE_FILE_MACHINE_AMD64' dll-info.txt
        grep -Fq 'IMAGE_FILE_DLL' dll-info.txt
        grep -Fxq '  Name: get_module' dll-info.txt
        grep -Fxq '  Name: ${phpLibrary}.dll' dll-info.txt
        runHook postCheck
      '';

      installPhase = ''
        runHook preInstall
        install -Dm644 php_yumemi.dll "$out/lib/php/extensions/php_yumemi.dll"
        mkdir -p "$out/share/licenses/php-yumemi"
        cp LICENSE.md docs/LICENSE_EXCEPTION.md docs/UDUNITS-COPYRIGHT "$out/share/licenses/php-yumemi/"
        runHook postInstall
      '';

      meta = meta // {
        description = "Windows x64 yumemi extension for PHP ${php.version} ${ts}, cross-compiled with xwin and LLVM";
      };
    };
in
lib.mergeAttrsList (
  lib.mapAttrsToList (name: php: {
    "${name}-windows-nts" = makePackage php "nts";
    "${name}-windows-ts" = makePackage php "ts";
  }) phpVersions
)
