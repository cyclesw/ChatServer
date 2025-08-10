{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  boost,
  openssl,
  grpc,
  cpprestsdk,
  protobuf,
}@ args:

stdenv.mkDerivation rec{
  pname = "etcd-cpp-apiv3";
  version = "unstable";

  src = fetchFromGitHub ({
    owner = "etcd-cpp-apiv3";
    repo = "etcd-cpp-apiv3";
    rev = "master";
    sha256 = "sha256-EmHT7G44t3LT0lZRe4/ngZX79UmbXiyIiWPdeVgHILI=";
  });

  buildInputs = [];
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ openssl boost grpc cpprestsdk protobuf];
}
