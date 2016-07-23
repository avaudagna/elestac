#!/bin/bash
# SisOp 1C2016
# Instalador de Elestac
# VamoACalmarno
# http://about.me/pacevedo

ORIG_PATH=$(pwd)
TP_PATH=$ORIG_PATH"/tp-2016-1c-Vamo-a-calmarno"
PATH_SWAP=$TP_PATH"/SWAP/"
USER="pabloxeven"
MAIL="pablo@xeven.com.ar"

function mostrar() {
  echo "$1"
  echo "$2"
}

function avance() {
  if [ $CONTAME -lt 100 ]
  then
    printf "\r\e[37;41m \e[?25l ${1}...        (${CONTAME} of 100) \e[0m"
    CONTAME=$(($CONTAME + 20))
    sleep .5
  fi
}

function bajar(){
  mostrar "" "Input your GIT credentials:"
  read -p " GIT user name: " GUSER;
  read -p " GIT user email: " GEMAIL;
  if [ -z "$GUSER" ]
  then
    GUSER="$USER"
  fi
  if [ -z "$GEMAIL" ]
  then
    GEMAIL="$MAIL"
  fi
  git config --global user.name $GUSER
  git config --global user.email $GEMAIL
  git clone "https://${USER}@github.com/sisoputnfrba/tp-2016-1c-Vamo-a-calmarno.git"
  git clone "https://${USER}@github.com/sisoputnfrba/so-commons-library.git"
  git clone "https://${USER}@github.com/sisoputnfrba/ansisop-parser.git"
  mostrar "All repos have been cloned."
}

function instalarLibs(){
  mostrar
  cd $ORIG_PATH"/so-commons-library"
  sudo make
  sudo make install
  cd $ORIG_PATH
  cd $ORIG_PATH"/ansisop-parser/parser"
  make all
  sudo make install
  cd $ORIG_PATH
  mostrar "All libraries have been installed."
}

function instalar {
  echo ""
  echo " .:: The system will now be compiled ::."
  echo ""
  declare -i CONTAME=19
  cd $TP_PATH"/CPU"
  gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o cpu implementation_ansisop.c libs/pcb_tests.c libs/socketCommons.c libs/stack.c libs/pcb.c libs/serialize.c cpu_init.c cpu.c -L/usr/lib -lcommons -lparser-ansisop -lm -w
  avance "CPU compiled successfully"
  cd $TP_PATH"/UMC/umc"
  gcc -I/usr/include/commons -I/usr/include/commons/collections -o umc umc.c -L/usr/lib -pthread -lcommons -w
  avance "UMC compiled successfully"
  cd $TP_PATH"/SWAP"
  gcc -I/usr/include/commons -I/usr/include/commons/collections -o swap socketCommons.c swap.c libs/serialize.c -L/usr/lib -lcommons -w
  avance "SWAP compiled successfully"
  cd $TP_PATH"/KERNEL"
  gcc -I/usr/include/parser -I/usr/include/commons -I/usr/include/commons/collections -o kernel socketCommons.c libs/stack.c libs/pcb_tests.c libs/pcb.c libs/serialize.c kernel.c -L/usr/lib -lcommons -lparser-ansisop -pthread -w
  avance "KERNEL compiled successfully"
  cd $TP_PATH"/CONSOLE"
  gcc -I/usr/include/commons -o ansisop console.c libs/socketCommons.c libs/serialize.c -L/usr/lib -lcommons -w
  cp ./ansisop /usr/bin/ansisop
  chmod +x /usr/bin/ansisop
  avance "CONSOLE compiled successfully"
  mostrar
  mostrar "All modules have been compiled."
  cd $TP_PATH
  sudo chmod -R a+rwX *
  cd ..
  chown -R utnso tp-2016-1c-Vamo-a-calmarno
}

function configurar {
  echo ""
  echo " .:: Answer these questions to setup & install ::."
  echo " Fields are optional. Leave any field empty to keep current settings."
  echo ""
  read -p " IP SWAP: " SWAP;
  read -p " SWAP OPEN PORT: " SWAP_P;
  read -p " IP UMC: " UMC;
  read -p " UMC OPEN PORT: " UMC_P;
  read -p " IP KERNEL: " KERNEL;
  read -p " KERNEL PORT FOR CONSOLES: " KERNEL_CONSOLE;
  read -p " KERNEL PORT FOR CPUS: " KERNEL_CPU;
  declare -i CONTAME=19
  mostrar
  ####################################### UMC #######################################
  avance "Creating config file for UMC"
  EL_FILE=$TP_PATH"/UMC/umc/configFile.dat"
  agregarCoso "IP_ESCUCHA" $EL_FILE $UMC
  agregarCoso "IP_SWAP" $EL_FILE $SWAP
  agregarCoso "PUERTO_SWAP" $EL_FILE $SWAP_P
  agregarCoso "PUERTO_ESCUCHA" $EL_FILE $UMC_P
  ####################################### CPU #######################################
  avance "Creating config file for CPU"
  EL_FILE=$TP_PATH"/CPU/setup.data"
  agregarCoso "IP_UMC" $EL_FILE $UMC
  agregarCoso "KERNEL_IP" $EL_FILE $KERNEL
  agregarCoso "PUERTO_KERNEL" $EL_FILE $KERNEL_CPU
  agregarCoso "PUERTO_UMC" $EL_FILE $UMC_P
  ####################################### SWAP #######################################
  avance "Creating config file for SWAP"
  EL_FILE=$TP_PATH"/SWAP/swapConf"
  agregarCoso "IP_SWAP" $EL_FILE $SWAP
  agregarCoso "PUERTO_ESCUCHA" $EL_FILE $SWAP_P
  ####################################### KERNEL #######################################
  avance "Creating config file for KERNEL"
  EL_FILE=$TP_PATH"/KERNEL/setup.data"
  agregarCoso "IP_UMC" $EL_FILE $UMC
  agregarCoso "KERNEL_IP" $EL_FILE $KERNEL
  agregarCoso "PUERTO_CPU" $EL_FILE $KERNEL_CPU
  agregarCoso "PUERTO_PROG" $EL_FILE $KERNEL_CONSOLE
  agregarCoso "PUERTO_UMC" $EL_FILE $UMC_P
  ####################################### CONSOLE #######################################
  avance "Creating config file for CONSOLE"
  EL_FILE=$TP_PATH"/CONSOLE/console.config"
  agregarCoso "IP_KERNEL" $EL_FILE $KERNEL
  agregarCoso "PUERTO_KERNEL" $EL_FILE $KERNEL_CONSOLE
  sudo rm -rf /usr/share/ansisop
  sudo mkdir /usr/share/ansisop
  sudo cp $EL_FILE /usr/share/ansisop/console.config
  #######################################################################################
  mostrar
  mostrar "All config files have been set."
}

function borrar(){
  mostrar "" " .:: Let the game begin ::."
  echo ""
  read -p "Are you sure you want to uninstall? [Y/N] " BLEH
  case $BLEH in
    [Yy]* ) otrave;;
    [Nn]* ) echo "I knew it :-D";;
    * ) echo "I will take that as a NO.";;
  esac
}

function otrave(){
	mostrar "" "Seriously, this will delete everything."
	read -p "Are you posta sure? [Y/N] " RESP
	case $RESP in
	[Yy]* ) mostrar "Killing me softly :-( "
		    sudo rm -rf $TP_PATH
		    mostrar "Threw it on the GROUND";;
	[Nn]* ) echo "Some men just want to watch the world burn";;
	* ) echo "Cool guys don't look at explosions.";;
	esac
}

function agregarCoso(){
    if [ -n "$3" ]
    then
      sed -i "/${1}/ c\\${1}=${3}" $2
    fi
  }

function forceRoot(){
  if [[ $(id -un) != "root" ]]
  then
   echo "If you are not root, you can't install the software, papa."
   sudo $0
   exit
  fi
}

clear
forceRoot
echo ""
printf " \e[31;4;1m -== SisOp 1C2016 - Elestac ==- \e[0m"
mostrar
options=("Download - Clone repositories" \
    "Install libs - Setup the parser and commons libraries" \
    "Setup - Create config files" \
    "Install - Compile the system" \
    "Clean - Delete Elestac system" "Quit")
PS3="Pick an option: "
select opt in "${options[@]}"; do 
    case "$REPLY" in
    1 ) bajar;;
    2 ) instalarLibs;;   
    3 ) configurar;;
    4 ) instalar;;
    5 ) borrar;;
    6 ) echo "#VamoACalmarno - SisterCall"; break;;
    *) echo "Invalid option. Try another one."; continue;;
    esac
done
