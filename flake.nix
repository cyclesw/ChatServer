{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";

  outputs = { self, nixpkgs, }:
    let
      project = "GateWayServer";

      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });

      brpc = pkgs: pkgs.callPackage ./thirdparty/brpc.nix {};

      cpprestsdk = pkgs: pkgs.callPackage ./thirdparty/cpprestsdk.nix {};
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
              pkg-config
              ninja

              protols

              websocketpp
              spdlog
              asio_1_10
              protobuf
              redis-plus-plus
              httplib
              jsoncpp
              gflags
              gtest
              (brpc pkgs)
              (etcd-cpp-apiv3 pkgs)
              (cpprestsdk pkgs)

              curl
              leveldb
              abseil-cpp
              openssl
              snappy
              zstd
           ];
          };
      });
    };
}

