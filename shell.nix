let np = (import <nixpkgs> {});
    upd = np.callPackage ./default.nix {};
in upd.overrideAttrs (o: {
  nativeBuildInputs = ( o.nativeBuildInputs or []) ++ [np.ruby];
})
