# Examples where one git merge algo works better than the other one

В директории 5 скриптов, соответствующих 5 ситуациям где лучше соответствующий алгоритм. Каждый скрипт создает репозиторий, в нем делаются какие-то коммиты и в результате показывается как всё хорошо получается с одной стратегией, и как криво получается (или не получается вообще) с другой.

Везде я вставлю конец прогона скрипта, чтобы было наглядно видно историю коммитов/конфликты слияния.

## ff-demo
Показывает когда fast-forward лучше ort.

Ситуация вида: была main ветка, от нее отвели ветку для новой фичи. В main ничего не добавлялось, можно спокойно влить фичу просто перенеся указатель ветки, не создавая коммитов слияния. Это будет чисто и понятно, а с ort получится лишняя возня. Не знаю что и комментировать.

С fast-forward история:
```sh
[7/7] Running merge: fast-forward
Обновление 44d4e2d..405c670
Fast-forward
 notes.txt | 1 +
 1 file changed, 1 insertion(+)

Merge complete. Final history:
* 405c670 (HEAD -> main, feature) Feature change
* 44d4e2d Initial commit
```

С ort история:
```sh
[7/7] Running merge: ort
Merge made by the 'ort' strategy.
 notes.txt | 1 +
 1 file changed, 1 insertion(+)

Merge complete. Final history:
*   92dac42 (HEAD -> main) Merge branch 'feature'
|\  
| * 37f3873 (feature) Feature change
|/  
* 9bb09e4 Initial commit
```

## octopus-demo
Показывает когда octopus лучше ort.

Ситуация: нам прислали несколько коммитов разные разработчики из своих веток. Мы знаем что они работали вообще над разными задачами с разными частями кода. Хотим разом протестировать изменения и влить их все вместе. Если вливать с octopus, то история коммитов отражает наши намерения. Если вливать дефолтной стратегией по одной ветке - будет мешанина в логе. octopus в таком случае лучше.

octopus:
```sh
[8/8] Merge complete

Final history:
*-.   ad2e4c2 (HEAD -> main) Merge branches 'feature-auth', 'feature-billing' and 'feature-notifications'
|\ \  
| | * 66ed948 (feature-notifications) Add notifications spec
| * | f1b0d28 (feature-billing) Add billing spec
| |/  
* / 5dc4cdd (feature-auth) Add authentication spec
|/  
* 920fbb1 Initial commit
```

multiple ort one by one:
```
[8/8] Merge complete

Final history:
*   e3904e1 (HEAD -> main) Merge branch 'feature-notifications'
|\  
| * 35aa9ff (feature-notifications) Add notifications spec
* |   4f10f1e Merge branch 'feature-billing'
|\ \  
| * | b00ece4 (feature-billing) Add billing spec
| |/  
* |   0857ae9 Merge branch 'feature-auth'
|\ \  
| |/  
|/|   
| * 03ccaf6 (feature-auth) Add authentication spec
|/  
* 5706b9a Initial commit
```

## ours-demo
Показывает когда ours лучше ort.

Ситуация представлена примерно такая: был код, в нем есть небольшая проблема. Мы работаем над какой-то новой функциональностью. Нашему коллеге для его задачи понадобилось срочно поправить баг. Он подложил какое-то решение и влил его в main, оно нам не подходит. Мы точно знаем что наше решение правильнее и поэтому вливаем актуальный main к нам со стратегией ours, чтобы в коде осталось наше исправление. Потом можем без конфликтов влить нашу ветку в main. Если же просто пытаться применить resolve (или ort) слияние нашей ветки в main - будет конфликт, который надо руками исправлять.

ours:
```sh
[7/7] Running merge: ort
Merge made by the 'ort' strategy.
 notes.txt | 1 +
 1 file changed, 1 insertion(+)

Merge complete. Final history:
*   92dac42 (HEAD -> main) Merge branch 'feature'
|\  
| * 37f3873 (feature) Feature change
|/  
* 9bb09e4 Initial commit
```

resolve:
```sh
[8/8] Conflict shown in division.py
a = float(input())
b = float(input())
if b == 0:
\<<<<<<< .merge_file_U0bp8k
    print("Error: division by zero")
\=======
    if a == 0:
        print("idk")
    else:
        print("\u221E")
\>>>>>>> .merge_file_Xs69sW
else:
    print(a / b)

Final history:
* 37f9a66 (enrichment) Handle division by zero with infinity/idk
| * 48caea5 (HEAD -> main) Merge branch 'fix-branch'
|/| 
| * 1365a9e (fix-branch) Handle division by zero with error message
|/  
* 89d9a70 Initial division script
```

## subtree-demo
Тут в общем предлагаемый [на kernel.org](https://www.kernel.org/pub/software/scm/git/docs/howto/using-merge-subtree.html) сценарий - желание использовать один репозиторий в составе нашего. Понятно что два репозитория это разные истории версий. Вот эта стратегия позволяет их корректно сливать. Иначе и непонятно как делать - я попробовал привести в пример копипасту соседнего репозитория на каждую новую версию, на которую мы хотим обновляться. Но кажется это не все потребности удовлетворяет - как минимум мы так как будто нарушаем авторские права.

Собственно, в примере мы добавляем core репозиторий в наш в качестве вложенной директории и обновляемся на новую версию core проекта ближе к концу. В одном случае через fetch и merge с subtree стратегией, в другом случае просто копипастой. В первом случае история сохраняется и видно что мы делаем обновление, во втором случае история нарушается и мы ее переписываем по своему усмотрению.

subtree:
```sh
[6/7] Showing final files
vendor/core/contract.txt
vendor/core/core.txt
[7/7] Showing history
*   21fa393 (HEAD -> main) Merge remote-tracking branch 'core/main'
|\  
| * 6bcd4c7 (core/main, core/HEAD) Core update: v3 text
| * eb78370 Core update: add contract
| * ce635dd Core update: v2 text
* | 2542a23 Initial subtree import of core
|\| 
| * 54d472c Initial core content
* 902234e Initial app commit
```

Копипаста руками и обычный merge с ort:
```sh
[6/7] Showing final files
vendor/core/contract.txt
vendor/core/core.txt
[7/7] Showing history
* 607073e (HEAD -> main) Update by copy-paste
* 5516a76 Initial copy-paste snapshot
* 95db4e8 Initial app commit
```

## ort-demo

Тут сравнение ort и resolve. 

Собственно, ситуация: мы поменяли какой-то код (например конфиг в реальной жизни), в соседней ветке коллега этот файл переименовал и переименование попало в main. ort это (переименование файла) обнаружит, распарсит и сможет влить нашу ветку без конфликтов, resolve на такое не способен и нам придется решать конфликт руками.

ort:
```sh
[8/8] Final history and file content
*   3a565bb (HEAD -> main) Merge branch 'edit-branch'
|\  
| * a644578 (edit-branch) Edit file-a on edit-branch
* |   9fa7457 Merge branch 'rename-branch'
|\ \  
| |/  
|/|   
| * 68a604a (rename-branch) Rename file-a to file-c
|/  
* c379903 Initial file-a

file-c content:
line 1 edited on edit-branch
line 2
```

resolve:
```sh
[7/8] Trying merge with -s resolve
resolve failed as expected
error: Merge requires file-level merging
Попытка тривиального слияния в индексе...
Не вышло.
Trying simple merge.
Simple merge failed, trying Automatic merge.
ERROR: file-a: Not handling case 7bba8c8e64b598d317cdf1bb8a63278f9fc241b1 ->  -> 94add0b4a00f1d5eb49a809145a5271324d02ea9
fatal: merge program failed
Сбой автоматического слияния; исправьте конфликты, затем зафиксируйте результат.
[8/8] Final history and file content
* 02a155c (edit-branch) Edit file-a on edit-branch
| * e1f48bd (HEAD -> main) Merge branch 'rename-branch'
|/| 
| * a7eee21 (rename-branch) Rename file-a to file-c
|/  
* d2381a5 Initial file-a

file-c content:
line 1
line 2
```