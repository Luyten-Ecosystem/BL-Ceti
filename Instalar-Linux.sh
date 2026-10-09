cd ~/Escritorio/BL-Ceti

# Comprobar herramientas
cc --version
cmake --version

# Configurar y compilar BL-Ceti
cmake -S . -B build
cmake --build build || exit 1

# Instalar el comando BL en tu usuario
install -Dm755 build/BL ~/.local/bin/BL

# Comprobar el comando
which BL
BL .C programa.bl


# Registrar BL-Ceti como tipo de archivo en Linux
mkdir -p ~/.local/share/mime/packages
mkdir -p ~/.local/share/applications

cat > ~/.local/share/mime/packages/bl-ceti.xml <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="text/x-bl-ceti">
        <comment>Archivo fuente BL-Ceti</comment>
        <glob pattern="*.bl"/>
    </mime-type>
</mime-info>
EOF

update-mime-database ~/.local/share/mime

echo "Asociación de archivos BL-Ceti registrada en Linux."
