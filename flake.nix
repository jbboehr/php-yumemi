# Copyright (c) anno Domini nostri Jesu Christi MMXXVI, John Boehr & contributors
#
# SPDX-License-Identifier: AGPL-3.0-only WITH romic-exception
{
  description = "Development environment and package for php-yumemi";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    systems.url = "github:nix-systems/default";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.systems.follows = "systems";
    };
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    gitignore = {
      url = "github:hercules-ci/gitignore.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    agent-badge = {
      url = "github:jbboehr/agent-badge.ts/master";
      inputs.flake-utils.follows = "flake-utils";
      inputs.gitignore.follows = "gitignore";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
      agent-badge,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = pkgs.lib;

        src = lib.cleanSourceWith {
          src = ./.;
          filter =
            path: type:
            let
              name = baseNameOf path;
              ignoredNames = [
                ".deps"
                ".direnv"
                ".libs"
                "Makefile"
                "Makefile.fragments"
                "Makefile.global"
                "Makefile.objects"
                "autom4te.cache"
                "build"
                "config.guess"
                "config.h"
                "config.h.in"
                "config.log"
                "config.nice"
                "config.status"
                "config.sub"
                "configure"
                "configure.ac"
                "include"
                "install-sh"
                "libtool"
                "ltmain.sh"
                "missing"
                "mkinstalldirs"
                "modules"
                "run-tests.php"
                "tmp"
                "tmp-php.ini"
                "tmp.md"
                "vendor"
              ];
              ignoredSuffixes = [
                "~"
                ".a"
                ".dep"
                ".diff"
                ".exp"
                ".la"
                ".lo"
                ".log"
                ".o"
                ".out"
              ];
            in
            lib.cleanSourceFilter path type
            && !(builtins.elem name ignoredNames)
            && !(lib.any (suffix: lib.hasSuffix suffix name) ignoredSuffixes);
        };

        phpVersions = {
          inherit (pkgs)
            php82
            php83
            php84
            php85
            ;
        };

        treefmt = treefmt-nix.lib.evalModule pkgs {
          projectRootFile = "flake.nix";
          programs.actionlint.enable = pkgs.stdenv.hostPlatform.isLinux;
          programs.nixfmt.enable = true;
        };

        makePackage =
          {
            buildPecl ? php.buildPecl,
            php,
            checkSupport ? false,
            requireQualification ? false,
          }:
          pkgs.callPackage ./nix/derivation.nix {
            inherit
              buildPecl
              checkSupport
              requireQualification
              php
              src
              ;
          };

        packagesByPhp = lib.mapAttrs (_: php: makePackage { inherit php; }) phpVersions;
        checksByPhp = lib.mapAttrs (
          _: php:
          makePackage {
            inherit php;
            checkSupport = true;
          }
        ) phpVersions;
        makeQualificationPhp =
          php:
          php.override {
            ztsSupport = true;
            phpAttrsOverrides = _final: previous: {
              configureFlags = previous.configureFlags ++ [ "--enable-debug" ];
            };
          };
        qualificationPhpVersions = {
          php82 = makeQualificationPhp pkgs.php82;
          php85 = makeQualificationPhp pkgs.php85;
        };
        qualificationChecks = lib.mapAttrs' (
          name: php:
          lib.nameValuePair "${name}-zts-debug" (
            (makePackage {
              inherit php;
              checkSupport = true;
              requireQualification = true;
            }).overrideAttrs
              {
                pname = "yumemi-${name}-zts-debug";
                YUMEMI_EXPECT_ZTS = "1";
                YUMEMI_EXPECT_DEBUG = "1";
              }
          )
        ) qualificationPhpVersions;
        sanitizerCheck =
          let
            php = phpVersions.php85;
            buildPecl = pkgs.callPackage "${nixpkgs}/pkgs/build-support/php/build-pecl.nix" {
              inherit php;
              stdenv = pkgs.clangStdenv;
            };
          in
          (makePackage {
            inherit buildPecl php;
            checkSupport = true;
          }).overrideAttrs
            (previous: {
              pname = "yumemi-php85-clang-sanitizers";
              CFLAGS = "-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined";
              LDFLAGS = "-fsanitize=address,undefined";
              preCheck = (previous.preCheck or "") + ''
                nm -D modules/yumemi.so > sanitizer-symbols
                grep -q '__asan_' sanitizer-symbols
                grep -q '__ubsan_' sanitizer-symbols
                export LD_PRELOAD=${pkgs.llvmPackages.compiler-rt}/lib/linux/libclang_rt.asan-x86_64.so
                export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:abort_on_error=1
                export UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1:print_stacktrace=1
                export USE_ZEND_ALLOC=0
                export USE_TRACKED_ALLOC=1
              '';
            });
        generatedSources =
          pkgs.runCommand "php-yumemi-generated-sources"
            {
              nativeBuildInputs = [
                phpVersions.php82
                pkgs.bison
                pkgs.flex
              ];
            }
            ''
              cp -R ${src} source
              chmod -R u+w source
              cd source
              bash scripts/generate-lexer.sh --check
              bash scripts/generate-parser.sh --check
              touch $out
            '';
        piePhar = pkgs.fetchurl {
          url = "https://github.com/php/pie/releases/download/1.4.10/pie.phar";
          hash = "sha256-uIeSI1yOgL5WhDbUywQ7Sf0YacibZOg9I+KIKuGdcKg=";
        };
        piePackaging =
          pkgs.runCommand "php-yumemi-pie-packaging"
            {
              nativeBuildInputs = [
                pkgs.autoconf
                pkgs.automake
                pkgs.git
                pkgs.gnumake
                pkgs.libtool
                pkgs.pkg-config
                pkgs.stdenv.cc
                phpVersions.php82
                phpVersions.php82.packages.composer
                phpVersions.php82.unwrapped.dev
              ];
            }
            ''
              export HOME="$TMPDIR/home"
              export COMPOSER_ROOT_VERSION=dev-develop
              mkdir -p "$HOME"
              cp -R ${src} source
              chmod -R u+w source
              cd source

              composer validate --strict --no-check-publish
              php ${piePhar} repository:add \
                --with-php-config=${phpVersions.php82.unwrapped.dev}/bin/php-config \
                path .
              if ! php ${piePhar} build \
                --make-parallel-jobs=1 \
                --with-php-config=${phpVersions.php82.unwrapped.dev}/bin/php-config \
                --with-phpize-path=${phpVersions.php82.unwrapped.dev}/bin/phpize \
                'jbboehr/php-yumemi:*@dev'; then
                find "$TMPDIR" -maxdepth 1 -type f -name 'pie_make_output_*' \
                  -exec sed -n '1,240p' {} \;
                exit 1
              fi

              module="$(find -L "$HOME/.config/pie" -type f -path '*/modules/yumemi.so' -print -quit)"
              test -n "$module"
              php -n -d "extension=$module" -r \
                'exit(extension_loaded("yumemi") ? 0 : 1);'
              touch $out
            '';
        devShellsByPhp = lib.mapAttrs (
          name: php:
          pkgs.mkShell {
            inputsFrom = [ packagesByPhp.${name} ];
            packages = [
              agent-badge.packages.${system}.default
              pkgs.bison
              pkgs.clang-tools
              pkgs.flex
              php.packages.composer
            ];

            shellHook = ''
              mkdir -p .direnv/include
              ln -sfn ${php.unwrapped.dev}/include/php .direnv/include/php
              export NO_INTERACTION=1
              export REPORT_EXIT_STATUS=1
            '';
          }
        ) phpVersions;
      in
      {
        packages = packagesByPhp // {
          default = packagesByPhp.php82;
        };
        checks =
          checksByPhp
          // {
            formatting = treefmt.config.build.check self;
            generated-sources = generatedSources;
          }
          // lib.optionalAttrs pkgs.stdenv.hostPlatform.isLinux {
            pie-packaging = piePackaging;
          }
          // lib.optionalAttrs (system == "x86_64-linux") (
            qualificationChecks
            // {
              php85-clang-sanitizers = sanitizerCheck;
            }
          );
        devShells = devShellsByPhp // {
          default = devShellsByPhp.php82;
        };
        formatter = treefmt.config.build.wrapper;
      }
    );
}
