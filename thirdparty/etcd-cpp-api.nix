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
  version = "unstable-2025-03-25";

  src = fetchFromGitHub ({
    owner = "etcd-cpp-apiv3";
    repo = "etcd-cpp-apiv3";
    rev = "master";
    sha256 = "sha256-l4csUmCp41WbVoOZkpVzqyE1nYu34wn6fsDGztWxQsE=";
  });

  buildInputs = [];
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ openssl boost grpc cpprestsdk protobuf];
}
