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
    owner = "cyclesw";
    repo = "etcd-cpp-apiv3";
    rev = "fixSubmoduleHeadless";
    sha256 = "sha256-vHL947OXKaTFjDGbXgstPuPY1kaPezdWRIA8EHxDfYA=";
  });

  buildInputs = [];
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ openssl boost grpc cpprestsdk protobuf];
}
