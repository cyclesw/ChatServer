{
  lib,
  stdenv,
  build2,
  fetchurl,
  # libmysqlclient,
  enableShared ? !stdenv.hostPlatform.isStatic,
  enableStatic ? !enableShared,
}:

stdenv.mkDerivation rec {
  pname = "libodb";
  version = "2.5.0";

  outputs = [
    "out"
    "dev"
    "doc"
  ];

  src = fetchurl {
    url = "https://pkg.cppget.org/1/stable/odb/libodb-${version}.tar.gz";
    hash = "sha256-cAA4pzxsvq0BESmxUDC3zdP3NRC2h/LEUEgI30IwRBs=";
  };

  nativeBuildInputs = [
    build2
  ];


  build2ConfigureFlags = [
    "config.bin.lib=${build2.configSharedStatic enableShared enableStatic}"
  ];

  doCheck = true;
}
