let np = (import <nixpkgs> {});
    upd = np.callPackage ./default.nix {};
in upd
