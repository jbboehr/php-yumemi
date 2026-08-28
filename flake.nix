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
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
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
            php,
            checkSupport ? false,
          }:
          pkgs.callPackage ./nix/derivation.nix {
            inherit php src checkSupport;
          };

        packagesByPhp = lib.mapAttrs (_: php: makePackage { inherit php; }) phpVersions;
        checksByPhp = lib.mapAttrs (
          _: php:
          makePackage {
            inherit php;
            checkSupport = true;
          }
        ) phpVersions;
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
              pkgs.bison
              pkgs.clang-tools
              pkgs.flex
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
          };
        devShells = devShellsByPhp // {
          default = devShellsByPhp.php82;
        };
        formatter = treefmt.config.build.wrapper;
      }
    );
}
