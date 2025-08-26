{
  description = "GateWayServer C/C++ dev env (nix flake)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      eachSystem = f: nixpkgs.lib.genAttrs systems (system: f (import nixpkgs {
        inherit system;
        overlays = [ self.overlays.default ];
      }));
    in
    {
      # ---------- overlay：把第三方包一次性注入 nixpkgs ----------
      overlays.default    = final: prev: {
        protobuf          = prev.protobuf_28;
        brpc              = final.callPackage ./thirdparty/brpc.nix          { };
        cpprestsdk        = final.callPackage ./thirdparty/cpprestsdk.nix    { };
        etcd-cpp-apiv3    = final.callPackage ./thirdparty/etcd-cpp-api.nix  { cpprestsdk = final.cpprestsdk; };
        odb               = final.callPackage ./thirdparty/odb.nix           { };
        libodb            = final.callPackage ./thirdparty/libodb.nix        { };
        libodb-mysql      = final.callPackage ./thirdparty/libodb-mysql.nix  { libodb = final.libodb; };
        libodb-boost      = final.callPackage ./thirdparty/libodb-boost.nix  { };
        libmysqlclient    = final.callPackage ./thirdparty/libmysqlclient.nix{ };
        libamqpcpp        = final.callPackage ./thirdparty/libamqpcpp.nix    { };
        elasticlient      = final.callPackage ./thirdparty/elasticlient.nix  { };
      };

      # ---------- devShell ----------
      devShells = eachSystem (pkgs: {
        default = pkgs.mkShell.override {
          stdenv = pkgs.clangStdenv; 
          } {
            packages = with pkgs; [
              # 编译&调试
              clang-tools pkg-config cmake gcc
              # 运行时 / 第三方库
              brpc cpprestsdk etcd-cpp-apiv3
              libodb libodb-mysql libodb-boost odb
              libmysqlclient libamqpcpp libev
              elasticlient cpr
              # 常用系统库
              protobuf redis-plus-plus httplib jsoncpp
              gflags gtest websocketpp spdlog asio_1_10
              # 隐形依赖
              curl leveldb abseil-cpp openssl snappy zstd
            ];
        };
      });
    };
}
