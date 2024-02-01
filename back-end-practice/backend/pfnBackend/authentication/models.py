from django.contrib.auth.models import User
from django.db import models

class Profile(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    nickname = models.CharField(max_length=30, blank=True)
    comment = models.CharField(max_length=100, blank=True)

    def save(self, *args, **kwargs):
        # If nickname is empty, set it to the user_id (username)
        if self.nickname == '':
            self.nickname = self.user.username
        super(Profile, self).save(*args, **kwargs)
        
    def __str__(self):
        return self.user.username