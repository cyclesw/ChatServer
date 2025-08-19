{ stdenv, build2, fetchurl
, lib
, libstudxml
, gmp
, enableShared ? !stdenv.hostPlatform.isStatic
, enableStatic ? !enableShared
}:

let
  libcutl = stdenv.mkDerivation rec {
    pname   = "libcutl";
    version = "1.11.0";   

    outputs = [ "out" "dev" ];

    src = fetchurl {
      url    = "https://pkg.cppget.org/1/stable/libcutl/libcutl-${version}.tar.gz";
      sha256 = "sha256-vsGDcxwcAbnfoanoNaJSJ8+2JtKcus0z33V34KUL4vk=";   
    };

    nativeBuildInputs = [ build2 ];

    build2ConfigureFlags = [
      "config.bin.lib=${build2.configSharedStatic enableShared enableStatic}"
    ];

    doCheck = true;
  };
in
stdenv.mkDerivation rec {
  pname   = "odb";
  version = "2.5.0";

  outputs = [
     "out"
     "dev"
     "doc"
  ];

  src = fetchurl {
    url    = "https://pkg.cppget.org/1/stable/odb/odb-${version}.tar.gz";
    sha256 = "sha256-kVEXKQf40BFqZCmyWdzJAM7QopkqXrYUS45MoFJfxkg=";   
  };

  nativeBuildInputs = [ build2 ];
  buildInputs       = [ 
    libstudxml
    libcutl 
    gmp
  ];   

  build2ConfigureFlags = [
    "config.bin.lib=${build2.configSharedStatic enableShared enableStatic}"
  ];

  doCheck = true;

meta = with lib; {
  description = "C++ Object-Relational Mapping (ORM)";
  longDescription = ''
    ODB is an open-source, cross-platform, and cross-database object-relational mapping (ORM) system for C++. 
    It allows you to persist C++ objects to a relational database without having to deal with tables, columns,
    or SQL and without manually writing any mapping code. ODB supports SQLite, PostgreSQL, MySQL,
    Oracle, and Microsoft SQL Server relational databases. 
    
    It also comes with optional profiles for Boost and Qt which allow you to seamlessly use value types,
    containers, and smart pointers from these libraries in your persistent C++ classes. 
    See Features for the detailed list of the supported functionality.
  '';
  homepage    = "https://www.codesynthesis.com/products/odb/";
  changelog   = "https://git.codesynthesis.com/cgit/odb/odb/tree/NEWS";
  license     = licenses.gpl2Only;         
  maintainers = with maintainers; [ cyclesw ];   
  platforms   = platforms.all;
};
}
