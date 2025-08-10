{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  boost177,
  websocketpp,
  zlib,
  openssl
}@ args:

stdenv.mkDerivation rec{
  pname = "cpprestsdk";
  version = "2.10.19";

  src = fetchFromGitHub ({
    owner = "microsoft";
    repo = "cpprestsdk";
    rev = "v${version}";
    sha256 = "sha256-zKam91x6wbG0vOxFXGQI/RKFf+fXGbn166YTjZUJ6yY=";
  });

  buildInputs = [];
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ openssl boost177 zlib websocketpp];

  cmakeFlags = [
    "-DWERROR=OFF"
    "-DCPPREST_EXCLUDE_WEBSOCKETS=ON"
    "-DCPPREST_EXCLUDE_COMPRESSION=ON"
  ];
}