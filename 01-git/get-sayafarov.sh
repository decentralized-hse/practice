function split_string() {
    local IFS='/'
    input_string=$1
    array=($input_string)

    for item in "${array[@]}"; do
        echo "$item"
    done
}

file_path=($(split_string "$1"))
cur_blob_hash=$2

if ! [[ -f "$cur_blob_hash" ]]; then
    echo ERROR: Root hash "$cur_blob_hash" doesn\'t exists 1>&2
    exit 1
fi

path_len="${#file_path[@]}"
declare -i cur_deep=0

for req_blob_name in "${file_path[@]}"; do
  suc=false
  cur_deep+=1
  if [[ "$cur_deep" == "$path_len" ]]; then
    while read -r blob_name blob_hash; do
      if [[ "$blob_name" == "$req_blob_name"/ ]] || [[ "$blob_name" == "$req_blob_name": ]]; then
        suc=true
        cur_blob_hash=$blob_hash
        break
      fi
    done < "$cur_blob_hash"
  else
    while read -r blob_name blob_hash; do
      if [[ "$blob_name" == "$req_blob_name"/ ]]; then
        suc=true
        cur_blob_hash=$blob_hash
        break
      fi
    done < "$cur_blob_hash"
  fi
  if [[ "$suc" == false ]]; then
    echo ERROR: No such file or directory "$1" 1>&2
    exit 1
  fi
done

cat "$cur_blob_hash"