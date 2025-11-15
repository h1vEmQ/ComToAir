# Инструкция по загрузке проекта на GitHub

## Шаг 1: Создание репозитория на GitHub

1. Откройте браузер и перейдите на https://github.com
2. Войдите в свой аккаунт (или создайте новый)
3. Нажмите кнопку **"+"** в правом верхнем углу
4. Выберите **"New repository"**
5. Заполните форму:
   - **Repository name**: `ComToAir` (или другое имя)
   - **Description**: `RS-232 to WiFi bridge for XIAO ESP32-C6`
   - Выберите **Public** или **Private**
   - **НЕ** ставьте галочки на "Initialize with README", "Add .gitignore", "Choose a license" (у нас уже есть эти файлы)
6. Нажмите **"Create repository"**

## Шаг 2: Подключение локального репозитория к GitHub

После создания репозитория GitHub покажет инструкции. Выполните следующие команды:

### Если репозиторий пустой (рекомендуется):

```bash
cd /Users/wind/Documents/PlatformIO/Projects/ComToAir

# Добавьте удаленный репозиторий (замените YOUR_USERNAME на ваш GitHub username)
git remote add origin https://github.com/YOUR_USERNAME/ComToAir.git

# Переименуйте ветку в main (если нужно)
git branch -M main

# Загрузите код на GitHub
git push -u origin main
```

### Альтернативный способ (через SSH):

Если у вас настроен SSH ключ:

```bash
git remote add origin git@github.com:YOUR_USERNAME/ComToAir.git
git branch -M main
git push -u origin main
```

## Шаг 3: Проверка

1. Обновите страницу репозитория на GitHub
2. Убедитесь, что все файлы загружены
3. Проверьте, что README.md отображается правильно

## Дополнительные команды Git

### Просмотр статуса:
```bash
git status
```

### Просмотр истории коммитов:
```bash
git log --oneline
```

### Добавление изменений и создание нового коммита:
```bash
git add .
git commit -m "Описание изменений"
git push
```

### Просмотр удаленных репозиториев:
```bash
git remote -v
```

## Решение проблем

### Если запрашивается пароль при push:

1. **Используйте Personal Access Token** вместо пароля:
   - GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
   - Generate new token
   - Выберите права: `repo`
   - Используйте токен как пароль

2. **Или настройте SSH ключ** (рекомендуется):
   - Следуйте инструкциям: https://docs.github.com/en/authentication/connecting-to-github-with-ssh

### Если возникла ошибка "remote origin already exists":

```bash
# Удалите существующий remote
git remote remove origin

# Добавьте заново
git remote add origin https://github.com/YOUR_USERNAME/ComToAir.git
```

### Если нужно изменить URL удаленного репозитория:

```bash
git remote set-url origin https://github.com/YOUR_USERNAME/ComToAir.git
```

## Полезные ссылки

- [GitHub Docs](https://docs.github.com/)
- [Git Handbook](https://guides.github.com/introduction/git-handbook/)
- [PlatformIO Documentation](https://docs.platformio.org/)

