{
  lib,
  stdenv,
  build2,
  fetchurl,
  libodb,
  boost177,
  # libmysqlclient,
  enableShared ? !stdenv.hostPlatform.isStatic,
  enableStatic ? !enableShared,
}:

stdenv.mkDerivation rec {
  pname = "libodb-boost";
  version = "2.5.0-b.27";

  outputs = [
    "out"
    "dev"
    "doc"
  ];

  src = fetchurl {
    url = "https://pkg.cppget.org/1/beta/odb/libodb-boost-${version}.tar.gz";
    hash = "sha256-tHArqiHNjPLCHuORuoHoAdxJIltVMsxg5PmvUjO2R/I=";
  };

  nativeBuildInputs = [
    build2
  ];
  buildInputs = [
    libodb
    boost177
  ];


  build2ConfigureFlags = [
    "config.bin.lib=${build2.configSharedStatic enableShared enableStatic}"
  ];

  doCheck = true;
}