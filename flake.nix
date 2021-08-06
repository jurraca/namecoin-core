{
  description = "(insert short project description here)";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-21.05";

  # Upstream source tree(s).
  inputs.namecoin-src = { url = "github:namecoin/namecoin-core"; flake = false; };
  inputs.gnulib-src = { url = git+https://git.savannah.gnu.org/git/gnulib.git; flake = false; };

  outputs = { self, nixpkgs, gnulib-src, namecoin-src }:
    let

      # Generate a user-friendly version numer.
      version = builtins.substring 0 8 namecoin-src.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f system);

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay ]; });

    in

    {

      # A Nixpkgs overlay.
      overlay = final: prev: {

        namecoin-core = with final; stdenv.mkDerivation rec {
          name = "namecoin-${version}";

          src = namecoin-src;

          buildInputs = [ autoconf automake python3 libtool boost libevent pkg-config hexdump ];

          configurePhase = ''
              ./autogen.sh
              ./configure --prefix=$out --without-bdb'';

          buildPhase = '' make -j 4'';
          installPhase = '' make install '';

          meta = {
            homepage = "https://namecoin.org";
            downloadPage = "https://namecoin.org/download";
            description = "a decentralized open source information registration and transfer system based on the Bitcoin cryptocurrency.";
          };
        };

      };

      # Provide some binary packages for selected system types.
      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) namecoin-core;
        });

      # The default package for 'nix build'. This makes sense if the
      # flake provides only one package or there is a clear "main"
      # package.
      defaultPackage = forAllSystems (system: self.packages.${system}.namecoin-core);

      # A NixOS module, if applicable (e.g. if the package provides a system service).
      nixosModules.namecoin-core =
        { pkgs, ... }:
        {
          nixpkgs.overlays = [ self.overlay ];

          environment.systemPackages = [ pkgs.namecoin-core ];

          #systemd.services = { ... };
        };

      # Tests run by 'nix flake check' and by Hydra.
      checks = forAllSystems (system: {
        inherit (self.packages.${system}) namecoin-core;

        # Additional tests, if applicable.
        test =
          with nixpkgsFor.${system};
          stdenv.mkDerivation {
            name = "namecoin-core-test-${version}";

            buildInputs = [ namecoin-core ];

            unpackPhase = "true";

            buildPhase = ''
              echo 'running some integration tests'
              [[ $(namecoin-core) = 'Hello, world!' ]]
            '';

            installPhase = "mkdir -p $out";
          };

        # A VM test of the NixOS module.
        vmTest =
          with import (nixpkgs + "/nixos/lib/testing-python.nix")
            {
              inherit system;
            };

          makeTest {
            nodes = {
              client = { ... }: {
                imports = [ self.nixosModules.namecoin-core ];
              };
            };

            testScript =
              ''
                start_all()
                client.wait_for_unit("multi-user.target")
                client.succeed("hello")
              '';
          };
      });

    };
}
