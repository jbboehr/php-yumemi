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
          programs.clang-format = {
            enable = true;
            package = pkgs.llvmPackages_21.clang-tools;
            includes = [
              "php_yumemi.h"
              "src/*.c"
              "src/*.h"
            ];
            excludes = [
              "src/parser/parser.c"
              "src/parser/parser.h"
              "src/parser/scanner.c"
              "src/parser/scanner.h"
              "src/parser/unicode_ranges.h"
            ];
          };
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
        devShellsByPhp = lib.mapAttrs (
          name: php:
          pkgs.mkShell {
            inputsFrom = [ packagesByPhp.${name} ];
            packages = [
              agent-badge.packages.${system}.default
              pkgs.bison
              pkgs.llvmPackages_21.clang-tools
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
        checks = import ./nix/checks.nix {
          inherit
            pkgs
            nixpkgs
            system
            src
            phpVersions
            makePackage
            ;
          formatting = treefmt.config.build.check self;
        };
        devShells = devShellsByPhp // {
          default = devShellsByPhp.php82;
        };
        formatter = treefmt.config.build.wrapper;
      }
    );
}
