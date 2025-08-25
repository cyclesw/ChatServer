{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  libcpr,
  jsoncpp,
}:

stdenv.mkDerivation rec{
  pname = "elasticlient";
  version = "master";

  src = fetchFromGitHub ({
    owner = "seznam";
    repo = "elasticlient";
    rev = version;
    sha256 = "sha256-PI+g/jv/enCyggv7Z5VSWEaQiL/2aNxI0NFSYbQUZn0=";
  });

  outputs = [
    "out"
    "dev"
  ];

  nativeBuildInputs = [ cmake ];
  buildInputs = [ ];
  propagatedBuildInputs = [  
      jsoncpp
      libcpr
  ];

  cmakeFlags = [
    (lib.cmakeBool "USE_ALL_SYSTEM_LIBS" true)
    (lib.cmakeBool "BUILD_ELASTICLIENT_TESTS" false)
    (lib.cmakeBool "BUILD_ELASTICLIENT_EXAMPLE" false)
  ];

  doCheck = true;

  patches = [
    ./elasticlient.patch
  ];
}
