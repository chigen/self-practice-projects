from django.urls import path
from .views import SignupView
from .views import UserDetailAndUpdateView
from .views import CloseAccountView

urlpatterns = [
    path('signup', SignupView.as_view(), name='signup'),
    path('users/<str:user_id>', UserDetailAndUpdateView.as_view(), name='user_detail_and_update'),
    path('close', CloseAccountView.as_view(), name='close_account'),
]