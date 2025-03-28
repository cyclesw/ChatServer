{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs, }:
    let
      project = "ChatClient";

      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });

      cpprestsdk = pkgs: pkgs.callPackage ./thirdparty/cpprestksdk.nix { };
      etcd-cpp-apiv3 = pkgs: pkgs.callPackage ./thirdparty/etcd-cpp-api.nix { 
        cpprestsdk = cpprestsdk pkgs;
      };
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            # Override stdenv in order to change compiler:
            stdenv = pkgs.clangStdenv;
          }
          {
            packages = with pkgs; [
              # env
              llvmPackages_latest.clang
              llvmPackages_latest.libllvm
              llvmPackages_latest.libcxx
              llvmPackages_latest.lldb
              llvmPackages_latest.libstdcxxClang

              clang-tools

              cmake
              ninja

              ### 3td lib
              spdlog
              gflags
              gtest
              protobuf
              (cpprestsdk pkgs)
              (etcd-cpp-apiv3 pkgs)

              ## dependence
              perl
              grpc
              abseil-cpp
              openssl
              zlib
            ];
              

            shellHook = ''
            '';
          };
      });
    };
}

