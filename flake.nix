{
  description = "C++ development environment for SFML";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = ["x86_64-linux" "aarch64-linux"];
    forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
  in {
    devShells = forEachSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = pkgs.mkShell {
          # nativeBuildInputs are tools needed at build time
          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
          ];

          # buildInputs are libraries needed for linking
          buildInputs = with pkgs; [
            # The specific X11 libraries missing in your error
            xorg.libX11
            xorg.libXrandr
            xorg.libXcursor
            xorg.libXi

            # Additional libraries commonly required when building SFML on Linux
            xorg.libXext
            libGL
            udev
            freetype
            openal
            flac
            libogg
            libvorbis
          ];
        };
      }
    );
  };
}
