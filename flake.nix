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
      odb = pkgs: pkgs.callPackage ./thirdparty/odb.nix {};
      libodb-mysql = pkgs: pkgs.callPackage ./thirdparty/libodb-mysql.nix {};
      libodb-boost = pkgs: pkgs.callPackage ./thirdparty/libodb-boost.nix {};
      mysqlclient = pkgs: pkgs.callPackage ./thirdparty/mysqlclient.nix {};

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
              gcc  # for odb compiler

              cmake
              pkg-config
              ninja
              (odb pkgs)

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
              libodb
              (libodb-mysql pkgs)
              (libodb-boost pkgs)
              (brpc pkgs)
              (etcd-cpp-apiv3 pkgs)
              (cpprestsdk pkgs)
              (mysqlclient pkgs)

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

