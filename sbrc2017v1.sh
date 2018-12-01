#!/bin/bash

# Terminal colors
declare -r NOCOLOR='\033[0m'
declare -r WHITE='\033[1;37m'
declare -r GRAY='\033[;37m'
declare -r BLUE='\033[0;34m'
declare -r CYAN='\033[0;36m'
declare -r YELLOW='\033[0;33m'
declare -r GREEN='\033[0;32m'
declare -r RED='\033[0;31m'
declare -r MAGENTA='\033[0;35m'

declare -a colors=( $NOCOLOR
                    $GRAY
                    $BLUE
                    $CYAN
                    $YELLOW
                    $GREEN
                    $RED
                    $MAGENTA
)

declare -i nRun=3 #35
declare -i prioritize=2

declare selectedCol="2"

# numero de estacoes com aplicacoes medicas
#declare -a nWBAN=(  "3"
#                    "6"
#                    "9")

# numero de estacoes com aplicacoes medicas
declare -a nWifi=(  "5"
                    "10")
                    #"15"
                    #"20"
                    #"25"
                    #"30"
                    #"35")

# Path of ns3 directory for running waf
declare NS3_PATH=/home/ubuntu/Documentos/ns-allinone-3.24.1/ns-3.24.1/
#declare NS3_PATH=~/repos3.24/tarballs/ns-allinone-3.24.1/ns-3.24.1/


#declare NS3_PATH=/Volumes/vms/ns3-mpaqm/
Run(){
	echo -e "${BLUE}Running Priorization Simluation... ${GRAY}\n"
  for (( nb=0; nb < ${#nWifi[@]}; nb++ ))
  do
	   for (( pt=0; pt < $prioritize; pt++ ))
	      do
		        # execution for delta 0 ... 30
		        for ((i=1; i < $nRun ; i++))
		          do

            		# nome arquivo com resultados conforme repetições
            		fileNamePrefix=${pt}"-"
                	#fileNamePrefix+=${nWBAN[$nb]}"-"
                	fileNamePrefix+=${nWifi[$nb]}
            		#fileNamePrefix+=${i}

            		NAME="${fileNamePrefix}"
            		echo -e "${colors[5]} - Run cenariowbanv3-21_12 ${NAME}"

            		./waf --run "scratch/cenariowbanv3-21_12 \
            		      --prioritize=${pt} \
                          --nWifi=${nWifi[$nb]} \
            		      --nRun=${i}" >> results-${NAME}.dat 2>&1

            done
                echo -e "${NOCOLOR}"

                # copia arquivo com informações de atraso
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_BE > results-${pt}-${nWifi[$nb]}-AC_BE.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VO > results-${pt}-${nWifi[$nb]}-AC_VO.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VI > results-${pt}-${nWifi[$nb]}-AC_VI.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_AM1 > results-${pt}-${nWifi[$nb]}-AC_AM1.dat

                # copia arquivo com informações de variação dos atrasos
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VarAM1 > results-${pt}-${nWifi[$nb]}-AC_VarAM1.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VarVO > results-${pt}-${nWifi[$nb]}-AC_VarVO.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VarVI > results-${pt}-${nWifi[$nb]}-AC_VarVI.dat
                cat results-${pt}-${nWifi[$nb]}.dat | grep AC_VarBE > results-${pt}-${nWifi[$nb]}-AC_VarBE.dat

      done
    done
    echo -e "${RED} Finished simulation! ${NOCOLOR}\n"
}

# abrir os diferentes arquivos de cada tipo de fluxo
SelecionaFluxoAdd(){
  for (( nb=0; nb < ${#nWifi[@]}; nb++ ))
  do
	   for (( pt=0; pt < $prioritize; pt++ ))
	      do

          # ATRASO MEDIO
          selectedCol="2"
          RES=$(Add results-${pt}-${nWifi[$nb]}-AC_AM1.dat)
          echo -e "AC_AM1 $RES" > resultsAtraso-${pt}-${nWifi[$nb]}.tr
          RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VO.dat)
          echo -e "AC_VO $RES" >> resultsAtraso-${pt}-${nWifi[$nb]}.tr
          RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VI.dat)
          echo -e "AC_VI $RES" >> resultsAtraso-${pt}-${nWifi[$nb]}.tr
          RES=$(Add results-${pt}-${nWifi[$nb]}-AC_BE.dat)
          echo -e "AC_BE $RES" >> resultsAtraso-${pt}-${nWifi[$nb]}.tr

          echo -e `cat resultsAtraso-${pt}-${nWifi[$nb]}.tr | awk '{a=$0;printf "%s ",a,$0}'` > resultsAtrasoFinal-${pt}-${nWifi[$nb]}.tr


          # TAXA DE ENTREGA
           selectedCol="6"
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_AM1.dat)
           echo -e "AC_AM1 $RES" > resultsTxEntrega-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VO.dat)
           echo -e "AC_VO $RES" >> resultsTxEntrega-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VI.dat)
           echo -e "AC_VI $RES" >> resultsTxEntrega-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_BE.dat)
           echo -e "AC_BE $RES" >> resultsTxEntrega-${pt}-${nWifi[$nb]}.tr

           echo -e `cat resultsTxEntrega-${pt}-${nWifi[$nb]}.tr | awk '{a=$0;printf "%s ",a,$0}'` > resultsTxEntregaFinal-${pt}-${nWifi[$nb]}.tr

          # TAXA DE PERDA
           selectedCol="7"
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_AM1.dat)
           echo -e "AC_AM1 $RES" > resultsTxPerda-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VO.dat)
           echo -e "AC_VO $RES" >> resultsTxPerda-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_VI.dat)
           echo -e "AC_VI $RES" >> resultsTxPerda-${pt}-${nWifi[$nb]}.tr
           RES=$(Add results-${pt}-${nWifi[$nb]}-AC_BE.dat)
           echo -e "AC_BE $RES" >> resultsTxPerda-${pt}-${nWifi[$nb]}.tr

            echo -e `cat resultsTxPerda-${pt}-${nWifi[$nb]}.tr | awk '{a=$0;printf "%s ",a,$0}'` > resultsTxPerdaFinal-${pt}-${nWifi[$nb]}.tr

     done
  done

  # ATRASO MEDIO

  # une resultados sem priorizacao
  echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS " > plotResultsAtraso-0.pr
  #cat resultsAtrasoFinal-0-5.tr resultsAtrasoFinal-0-10.tr resultsAtrasoFinal-0-15.tr resultsAtrasoFinal-0-20.tr resultsAtrasoFinal-0-25.tr resultsAtrasoFinal-0-30.tr resultsAtrasoFinal-0-35.tr >> plotResultsAtraso-0.pr
  cat resultsAtrasoFinal-0-5.tr resultsAtrasoFinal-0-10.tr >> plotResultsAtraso-0.pr

  # une resultados com priorizacao
  echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS " > plotResultsAtraso-1.pr
  #cat resultsAtrasoFinal-1-5.tr resultsAtrasoFinal-1-10.tr resultsAtrasoFinal-1-15.tr resultsAtrasoFinal-1-20.tr resultsAtrasoFinal-1-25.tr resultsAtrasoFinal-1-30.tr resultsAtrasoFinal-1-35.tr >> plotResultsAtraso-1.pr
  cat resultsAtrasoFinal-1-5.tr resultsAtrasoFinal-1-10.tr >> plotResultsAtraso-1.pr

  # TAXA DE ENTREGA

  # une resultados sem priorizacao
   echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS " > plotResultsTxEntrega-0.pr
 #  cat resultsTxEntregaFinal-0-5.tr resultsTxEntregaFinal-0-10.tr resultsTxEntregaFinal-0-15.tr resultsTxEntregaFinal-0-20.tr resultsTxEntregaFinal-0-25.tr resultsTxEntregaFinal-0-30.tr resultsTxEntregaFinal-0-35.tr >> plotResultsTxEntrega-0.pr
  cat resultsTxEntregaFinal-0-5.tr resultsTxEntregaFinal-0-10.tr >> plotResultsTxEntrega-0.pr

  # une resultados com priorizacao
   echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS "> plotResultsTxEntrega-1.pr
  # cat resultsTxEntregaFinal-1-5.tr resultsTxEntregaFinal-1-10.tr resultsTxEntregaFinal-1-15.tr resultsTxEntregaFinal-1-20.tr resultsTxEntregaFinal-1-25.tr resultsTxEntregaFinal-1-30.tr resultsTxEntregaFinal-1-35.tr >> plotResultsTxEntrega-1.pr
   cat resultsTxEntregaFinal-1-5.tr resultsTxEntregaFinal-1-10.tr >> plotResultsTxEntrega-1.pr

  # TAXA DE PERDA

  # une resultados sem priorizacao
   echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS " > plotResultsTxPerda-0.pr
   #cat resultsTxPerdaFinal-0-5.tr resultsTxPerdaFinal-0-10.tr resultsTxPerdaFinal-0-15.tr resultsTxPerdaFinal-0-20.tr resultsTxPerdaFinal-0-25.tr resultsTxPerdaFinal-0-30.tr resultsTxPerdaFinal-0-35.tr >> plotResultsTxPerda-0.pr
   cat resultsTxPerdaFinal-0-5.tr resultsTxPerdaFinal-0-10.tr >> plotResultsTxPerda-0.pr

  # une resultados com priorizacao
   echo -e "#NOS #AC_AM #LI #LS #AC_VO #LI #LS #AC_VI #LI #LS #AC_BE #LI #LS " > plotResultsTxPerda-1.pr
 #  cat resultsTxPerdaFinal-1-5.tr resultsTxPerdaFinal-1-10.tr resultsTxPerdaFinal-1-15.tr resultsTxPerdaFinal-1-20.tr resultsTxPerdaFinal-1-25.tr resultsTxPerdaFinal-1-30.tr resultsTxPerdaFinal-1-35.tr >> plotResultsTxPerda-1.pr
   cat resultsTxPerdaFinal-1-5.tr resultsTxPerdaFinal-1-10.tr >> plotResultsTxPerda-1.pr
}


# executa media. sd e erro para IC
Add(){
  if [ "${1}" == "" ]; then
   echo "file does not exist!"
   exit 0
  fi
  FILEP=$1 # nome arquivo por parametro

  MED=$(CalculaMedia ${FILEP})
  DVP=$(CalculaDesvioPadrao ${FILEP})
  ER=$(CalculaErro ${FILEP})

  LI=`echo "($MED - $ER)" | bc | gawk '{printf "%.3f", $0}'`
  LS=`echo "($MED + $ER)" | bc | gawk '{printf "%.3f", $0}'`
  echo -e "$MED $LI $LS"
}

CalculaMedia(){

  if [ "${1}" == "" ]; then
   echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=`cat ${FILEP} | gawk -v row=$selectedCol '{s+=$row;}END{printf "%f",s/NR;}'`
  echo $mean
}

CalculaDesvioPadrao(){

  if [ "${1}" == "" ]; then
    echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  mean=$(CalculaMedia ${FILEP})
  std=`gawk -v med=$mean -v row=$selectedCol '{aux=$row-med; stDev+=aux*aux;}END{ printf "%f", sqrt(stDev/(NR-1));}' ${FILEP}`
  echo $std
}

CalculaErro(){

  if [ "${1}" == "" ]; then
    echo "file does not exist!"
   exit 0
  fi
  FILEP=$1
  #Number of rows
  N=`gawk 'END{printf("%d",NR)}' ${FILEP}`
  if [ ${N} -gt 3 ]; then
    Z=1.96
    mean=$(CalculaMedia ${FILEP})
    std=$(CalculaDesvioPadrao ${FILEP})
    errop=`gawk -v stdev=$std -v z=$Z 'END{printf("%f",(z*stdev)/sqrt(NR))}' ${FILEP}`
  fi
  echo "$errop"
}

# limpa arquivos
Clean(){

    # cd ${NS3_PATH}

    if [ "$1" == "" ]; then
        Clean "tmp"
        Clean "dat"
        Clean "pcap"
        Clean "plt"
        Clean "tr"
        Clean "xr"
    else
        count=`find ./ -maxdepth 1 -name "*.${1}" | wc -l`
        if [ ${count} != 0 ]; then
            rm *.${1}
            echo "Removed ${count} .${1} files"
        fi
    fi

}

# Print usage instructions
ShowUsage()
{
    echo -e "${GRAY}\n"
    echo "Script to run Network Simulator 3 simulations"
    echo "Usage: ./sbrc2017.sh <COMMAND> [OPTION]"
    echo "Commands:"
    echo "   -r|--run                     Run simulation"
    echo "       OPTIONS: dataFileName    Run simulation and save data in file named <dataFileName>"
    echo "   -c|--clean                   Clean out the .xml animation and .data files"
    echo "       OPTIONS: file extension  Clean out .<file extension> files"
    echo "   -h|--help                    Show this help message"
    echo "Examples:"
    echo "    ./sbrc2017.sh -r"
    echo "    ./sbrc2017.sh -r trace_"
    echo "    ./sbrc2017.sh -c"
    echo "    ./sbrc2017.sh -c xml"
    echo -e "${WHITE}\n"
}

main()
{
  # $1 parametro 1 , $2 parametro 2 ...

    case "$1" in
        '-r'|'--run' )
            Run $2
        ;;

        '-c'|'--clean' )
            Clean $2
        ;;

        '-s'|'--select' )
            SelecionaFluxoAdd $2
        ;;

        *)
            ShowUsage
            exit 1
        ;;
    esac

    exit 0
}

main "$@"
