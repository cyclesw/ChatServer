{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  gtest,
  gperftools,
  gflags,
  openssl,
  zlib,
  protobuf_28,
  leveldb,
}:

stdenv.mkDerivation rec{
  pname = "brpc";
  version = "1.14.1";

  src = fetchFromGitHub ({
    owner = "apache";
    repo = "brpc";
    rev = version;
    sha256 = "sha256-xOsc9T5eXa+tQFnGBr1W/XlancJMO0uND/1l2yqGRVw=";
  });

  outputs = [
    "out"
    "dev"
  ];

  nativeBuildInputs = [ cmake ];
  buildInputs = [ gtest ];
  propagatedBuildInputs = [ openssl zlib leveldb protobuf_28 gflags gperftools ];

  cmakeFlags = [
    (lib.cmakeBool "DOWNLOAD_GTEST" false)
  ];

  doCheck = true;

  patches = [
  ];

  preFixup = ''
      substituteInPlace "$out/lib/pkgconfig/brpc.pc" \
        --replace 'includedir=''${prefix}//' 'includedir=/' \
        --replace 'libdir=''${prefix}//' 'libdir=/'
  '';


  meta = with lib; {
    description = "an Industrial-grade RPC framework using C++ Language";
    homepage   = "https://github.com/apache/brpc";
    license     = licenses.asl20;
    platforms   = platforms.all;
  };
}

