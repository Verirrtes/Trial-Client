# Trial Client — исходный код

Это форк BestClient/DDNet. В репозитории лежит исходный код Trial Client.

## Как залить на GitHub

1. Создай на GitHub пустой репозиторий (без README и .gitignore).
2. В этой папке выполни:

```bat
git remote add origin https://github.com/<твой-логин>/<имя-репозитория>.git
git branch -M main
git push -u origin main
```

Если Git попросит авторизацию — используй GitHub CLI (`gh auth login`) или
personal access token вместо пароля.

## Как собрать (Windows)

1. Установи Visual Studio 2022 с C++ tools, CMake и Ninja.
2. Подтяни сабмодуль:

```bat
git submodule update --init --recursive
```

3. Настрой сборку:

```bat
configure.bat
```

4. Собери клиент:

```bat
build_client.bat
```

5. Упакуй релиз:

```bat
package_client.bat
```

Готовый exe: `build\Trial.exe`
Архив: `dist\Trial-1.0.0-win64.zip`

## Важно

- `noxy-libs/ffmpeg-msvc` — предсобранные FFmpeg-библиотеки, нужны для
  Windows-сборки. Один файл `.lib` весит около 31 МБ, поэтому заливай репозиторий
  через `git push`, а не через веб-интерфейс GitHub (там лимит 25 МБ на файл).
- `ddnet-libs` — git-сабмодуль, его содержимое не входит в этот репозиторий.
- Не удаляй `LICENSE` и `license.txt`: проект основан на DDNet/BestClient.
