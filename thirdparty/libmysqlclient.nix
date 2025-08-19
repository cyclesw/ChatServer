{
  lib,
  stdenv,
  build2,
  fetchurl,
  mariadb,
  openssl_1_1,
  libz,
  # libmysqlclient,
  enableShared ? !stdenv.hostPlatform.isStatic,
  enableStatic ? !enableShared,
}:

stdenv.mkDerivation rec {
  pname = "mysqlclient";
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
}
