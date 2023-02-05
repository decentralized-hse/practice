#!/bin/bash

DATA_FILE_SIZE=$(wc -c < $1)
CHUNKS_NUMBER=$(( $DATA_FILE_SIZE / 1024 )) 
set -e

compute_offset() {
	if [[ $1 -eq 0 ]] 
	then
		printf $(( $2 * 2 ))
	else 
		first=$(( $1 - 1 ))
		second1=$(( $2 * 2 ))
		second2=$(( $2 * 2 + 1 ))
		printf $(( ( $(compute_offset $first $second1) + $(compute_offset $first $second2) ) / 2 ))
	fi
}

compute_parent_hash() {
	printf "$1\n$2\n" | sha256sum | cut -b -64
}

compute_leaf_hash() {
	dd skip=$(( $2 * 1024 )) count=1024 status=none if=$1 bs=1 | sha256sum | cut -b -64
}

decompose() {
	CURR=$1
	DECOMPOSITION=()
	COUNTER=1
	while [[ $CURR > 0 ]]
	do
		R=$(( $CURR % 2 ))
		if [[ $R -eq 1 ]]
		then
			DECOMPOSITION=( "${DECOMPOSITION[@]}" "$COUNTER" )	
		else
			DECOMPOSITION=( "${DECOMPOSITION[@]}" "0" )
		fi
		CURR=$(( $CURR / 2 ))
		COUNTER=$(( $COUNTER * 2 ))
	done
	printf '%s ' "${DECOMPOSITION[@]}" | rev
}


find_subtree() {
	COUNTER=0
	SUM=0
	for i in $(decompose $CHUNKS_NUMBER)
	do
		SUM=$(( $SUM + $i ))
		COUNTER=$i
		if [[ $1 -lt $SUM ]] 
		then 
			break 
		fi
	done
	echo "$(echo "(l($COUNTER)/l(2))" | bc -l)/1" | bc
}

verify_uncle_hashes() {
	CURR_OFFSET=$(( $2 * 2 ))
	CURR_HASH=$(compute_leaf_hash $1 $2)
	CURR_POWER=2
	CURR_LEVEL=0
    PROOF=$1.$2.proof
	SIZEOF_FILE=$(wc -c < $PROOF)
	
	if [[ $(( $SIZEOF_FILE % 65 )) -ne 0 ]]
	then
		return 1
	fi	
	for i in $(seq 0 $(( $SIZEOF_FILE / 65 - 1 )))
	do
		UNCLE_HASH=$(dd skip=$(( $i * 65 )) count=64 status=none if=$PROOF bs=1)
		BIAS=$(compute_offset $CURR_LEVEL 0)
		if [[ $(( ( ($CURR_OFFSET - $BIAS) / $CURR_POWER ) % 2 )) -eq 1 ]]
		then
			CURR_HASH=$(compute_parent_hash $UNCLE_HASH $CURR_HASH)
			CURR_OFFSET=$(( ($CURR_OFFSET + ($CURR_OFFSET - $CURR_POWER)) / 2 ))	
		else
			CURR_HASH=$(compute_parent_hash $CURR_HASH $UNCLE_HASH)
			CURR_OFFSET=$(( ($CURR_OFFSET + ($CURR_OFFSET + $CURR_POWER)) / 2 ))
		fi
		CURR_POWER=$(( $CURR_POWER * 2 ))
		CURR_LEVEL=$(( $CURR_LEVEL + 1 ))
	done

	SUBTREE_OFFSET=$(find_subtree $2)

	if [[ $CURR_HASH == $(dd skip=$(( $SUBTREE_OFFSET * 65 )) count=64 status=none if=$1.peaks bs=1) ]]
       	then 
		return 0 
	else
		return 1
	fi
}

verify_peaks() {
	if [[ $(sha256sum $1.peaks | cut -d ' ' -f 1) -eq $(dd count=64 status=none if=$1.root bs=1) ]]
	then
		return 0
	else
		return 1
	fi	
}

echo "Verifying peaks"
if ! verify_peaks $1
then
	echo "Invalid peaks"
	exit 1
fi
echo "Verifying uncle hashes"
if ! verify_uncle_hashes $1 $2
then
	echo "Invalid uncle hashes"
	exit 1
fi
echo "All done"
exit 0
