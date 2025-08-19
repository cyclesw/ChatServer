{
  lib,
  stdenv,
  build2,
  fetchurl,
  libodb,
  mariadb,
  openssl_1_1,
  libz,
  # libmysqlclient,
  enableShared ? !stdenv.hostPlatform.isStatic,
  enableStatic ? !enableShared,
}:

let
  libmysqlclient = stdenv.mkDerivation rec {
    pname = "libmysqlclient";
    version = "8.0.15+18";

    outputs = [
      "out"
      "dev"
      "doc"
    ];

    src = fetchurl {
      url = "https://pkg.cppget.org/1/stable/mysql/libmysqlclient-${version}.tar.gz";
      hash = "sha256-LJNn6YJlvC8K269xF0Bm40x35dHJ447ZTd37QzD/2EI=";
    };
    nativeBuildInputs = [
      build2
    ];

    buildInputs = [
      openssl_1_1
      libz
    ];

    doCheck = true;
  };
in

stdenv.mkDerivation rec {
  pname = "libodb-mysql";
  version = "2.5.0";

  outputs = [
    "out"
    "dev"
    "doc"
  ];

  src = fetchurl {
    url = "https://pkg.cppget.org/1/stable/odb/libodb-mysql-${version}.tar.gz";
    hash = "sha256-RWvCYksjKhBmzCJQOhmwSS0Xtb1C7vOw26fqhceOcF8=";
  };

  nativeBuildInputs = [
    build2
  ];
  buildInputs = [
    libodb
    libmysqlclient
    openssl_1_1
  ];


  build2ConfigureFlags = [
    "config.bin.lib=${build2.configSharedStatic enableShared enableStatic}"
  ];

  doCheck = true;
}
