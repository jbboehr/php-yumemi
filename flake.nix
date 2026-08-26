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

        src = lib.cleanSource ./.;

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
        devShellsByPhp = lib.mapAttrs (
          name: php:
          pkgs.mkShell {
            inputsFrom = [ packagesByPhp.${name} ];
            packages = [ pkgs.clang-tools ];

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
        checks = checksByPhp // {
          formatting = treefmt.config.build.check self;
        };
        devShells = devShellsByPhp // {
          default = devShellsByPhp.php82;
        };
        formatter = treefmt.config.build.wrapper;
      }
    );
}
