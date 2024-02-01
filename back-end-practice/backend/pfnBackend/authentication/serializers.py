from django.contrib.auth.models import User
from django.contrib.auth.password_validation import validate_password
from rest_framework import serializers
from .models import Profile
import re

class UserSerializer(serializers.ModelSerializer):
    user_id = serializers.CharField(source='username', required=True, min_length=6, max_length=20)
    password = serializers.CharField(write_only=True, required=True, min_length=8, max_length=20)
    nickname = serializers.CharField(source='profile.nickname', allow_blank=True, required=False)
    comment = serializers.CharField(source='profile.comment', allow_blank=True, required=False)

    class Meta:
        model = User
        fields = ('user_id', 'password', 'nickname', 'comment')

    def create(self, validated_data):

        user = User.objects.create_user(
            username=validated_data['username'],
            password=validated_data['password']
        )
        Profile.objects.create(
            user=user,
            nickname=validated_data.get('nickname', user.username),
            comment=validated_data.get('comment', '')
        )
        return user
    
    def update(self, instance, validated_data):
        # Update the user instance
        # print(f"validated_data is {validated_data}")
        # print(f"instance is {instance.profile}")
        instance.username = validated_data.get('username', instance.username)
        instance.save()

        profile_data = validated_data.get('profile', {})
        profile = instance.profile
        # print(f"profile_data is {profile_data}")
        # Update the Profile instance
        if 'nickname' in profile_data:
            profile.nickname = profile_data.get('nickname', profile.nickname)
        if 'comment' in profile_data:
            profile.comment = profile_data.get('comment', profile.comment)
        # print(f"profile has {profile.nickname} and {profile.comment}")
        profile.save()

        return instance

    def validate_user_id(self, value):
        if not re.match('^[A-Za-z0-9]*$', value):
            # print("user_id is invalid")
            raise serializers.ValidationError("user_id must be alphanumeric and between 6 to 20 characters.")
        # print("user_id is valid")
        return value

    def validate_password(self, value):
        # validate_password will check Django's default 
        validate_password(value)

        # Additional validation for password from the requirement
        if not re.match('^[ -~]{8,20}$', value):
            # print("password is invalid")
            raise serializers.ValidationError("password must be 8 to 20 ASCII characters (without spaces).")
        # print("password is valid")
        return value
