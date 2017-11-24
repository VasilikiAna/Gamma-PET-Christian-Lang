#! /bin/bash
## Job parameters
# Number of last job index
n=12
# Start counting from:
i=1
# other user parameters
simulationName='Test-Lang'
isotope='Na-22' 
outputdir=/project/med2/V.Anagnostatou/MEGAlib-gamma-PET/Reconstruction/${simulationName}
# creating and cleaning
mkdir ./log
mkdir ./err
mkdir ${outputdir}


echo "Cleaning up..."
rm -f ./log/test_${isotope}_*
rm -f ./err/test_${isotope}_*
rm -f job_Test_Lang_${isotope}_${i}*
rm -f Test_Lang_${isotope}_${i}*

while [[ $i -le $n ]]
do
echo "Submitting job No:${i}"
cat > job_test_Lang_${isotope}_${i}.sh << EOF
#!/bin/bash
#Job number $i

#SBATCH --job-name='Test-Lang_${isotope}_${i}'
#SBATCH --workdir=$PWD/
#SBATCH  --mem=5000
#SBATCH --partition=medcl
##SBATCH --partition=medcl-pri
##SBATCH  --partition=medbighmem
#SBATCH --output=$PWD/log/Test-Lang_${isotope}_${i}.log
#SBATCH --error=$PWD/err/Test-Lang_${isotope}_${i}.err

source /etc/profile.d/modules.sh
module load slurm/16.05.2

cat > Test_Lang_${isotope}_${i}.source << EOL
#source file ${i}

Version             2.0
Geometry            $PWD/MyGeoSetUpNew.geo.setup
CheckForOverlaps    1000 0.001

#-------------------------Physics lists----------------------------------------- 

PhysicsListEM         Livermore
PhysicsListHD         qgsp-bic-hp
DecayMode             Normal
  
#-------------------------Storing option----------------------------------------

StoreSimulationInfo                  all
StoreCalibrated                      true 
StoreSimulationInfoIonization        false
StoreOnlyTriggeredEvents             true
DiscretizeHits                       true

#--------------------------Regions----------------------------------------------
#DefaultRangeCut   0.05
#---------------------------Output file-----------------------------------------

Run                        		FirstRun
FirstRun.FileName               ${outputdir}/Na22-2mm-3Scat-1Abs-Cluster${i}
FirstRun.ActivationSources	  Na22.dat
FirstRun.Triggers               35000


EOL

source /project/med2/V.Anagnostatou/MEGAlib/bin/source-megalib.sh

cosima -s ${i} -v 0 Test_Lang_${isotope}_${i}.source

EOF
chmod 777 job_Test_Lang_${isotope}_${i}.sh
sbatch job_Test_Lang_${isotope}_${i}.sh
	((i++))
done 
