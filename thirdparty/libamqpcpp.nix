{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  openssl,
  libev,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "libamqpcpp";
  version = "4.3.27";

  src = fetchFromGitHub {
    owner = "CopernicaMarketingSoftware";
    repo = "AMQP-CPP";
    rev = "v${finalAttrs.version}";
    sha256 = "sha256-iaOXdDIJOBXHyjE07CvU4ApTh71lmtMCyU46AV+MGXQ=";
  };

  cmakeFlags = [
    "-DAMQP-CPP_BUILD_SHARED=ON"
    "-DAMQP-CPP_LINUX_TCP=ON"
  ];

  nativeBuildInputs = [ cmake ];

  buildInputs = [ openssl libev ];

  enableParallelBuilding = true;
  doCheck = true;

  meta = {
    description = "Library for communicating with a RabbitMQ server";
    homepage = "https://github.com/CopernicaMarketingSoftware/AMQP-CPP";
    license = lib.licenses.asl20;
    maintainers = with lib.maintainers; [ cyclesw ];
    platforms = lib.platforms.all;
  };
})