{ stdenv, fetchFromGitHub }:
let
  lua = stdenv.mkDerivation {
      pname = "lua";
      version = "5.4.0";
      src = builtins.fetchurl {
        url = "https://www.lua.org/ftp/lua-5.4.0.tar.gz";
        sha256 = "0a3ysjgcw41x5r1qiixhrpj2j1izp693dvmpjqd457i1nxp87h7a";
      };
      stripAllList = [ "bin" ];
      postPatch = ''
        sed -i src/Makefile -e "s/^AR= ar/AR= ''$AR/"
        sed -i src/Makefile -e '/^CC=/d' -e '/^RANLIB=/d'
        sed -i src/luaconf.h -e '/LUA_USE_DLOPEN/d' -e '/LUA_USE_READLINE/d'
      '';
      makeFlags = ["linux"
                   "INSTALL_TOP=${placeholder "out"}"
                  ];
    };

  inspect_lua = fetchFromGitHub {
    repo = "inspect.lua";
    owner = "kikito";
    name = "inspect.lua";
    rev = "b611db6bfa9c12ce35dd4972032fbbd2ad5ba965";
    sha256 = "04w6r7f4rnl6s3z76qz7qslp2lr0yy8b7gvdcclnhvpy8dz0jqhr";
  };

  json_lua = fetchFromGitHub {
    owner = "rxi";
    repo = "json.lua";
    name = "json.lua";
    rev = "dbf4b2dd2eb7c23be2773c89eb059dadd6436f94";
    sha256 = "16yzbyp296abirl77xk3fw5jqgcjf3frmwxph22sfxam8npkxcq6";
  };

  fennel = fetchTarball {
    name = "fennel-0.9.1";
    url = "https://fennel-lang.org/downloads/fennel-0.9.1.tar.gz";
    sha256 = "0m2al5j0nf8nydrs6yiif3zfwrfa68r97scj6kw4ysv2h4z6al5r";
  };

in stdenv.mkDerivation {
  name = "upd";
  src = ./.;
  CFLAGS = "-I${lua}/include";
  LDFLAGS = "-L${lua}/lib";
  depsBuildHost = [lua];
  LUA = "${lua}/bin/lua";
  LUA_PATH = "${inspect_lua}/?.lua;${json_lua}/?.lua;${fennel}/?.lua";
  FENNEL_LUA = "${fennel}/fennel";
  doCheck = true;
  checkPhase = "make test";
  installFlags = ["DESTDIR=$(out)"];
}
